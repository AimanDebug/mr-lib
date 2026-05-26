#include "reducer.h"

#include <stdlib.h>
#include <string.h>
#include <threads.h>

#include "protocol.h"
#include "queue.h"
#include "utils.h"

#define HASH_TABLE_SIZE 1024

/**
 * @brief Node for grouping values by token.
 */
typedef struct token_node {
  char* token;
  mr_value_t* values;
  size_t count;
  size_t capacity;
  struct token_node* next;
} token_node_t;

/**
 * @brief Arguments passed to each reducer worker thread.
 */
typedef struct {
  mr_queue_t* queue;
  mr_reducer_t reducer;
  void* user_arg;
  mr_log_file_t* log_file;
  mtx_t* stdout_mutex;
  size_t* results_count;
  char thread_name[32];
} reducer_thread_arg_t;

/**
 * @brief Arguments passed to the reducer emit function.
 */
typedef struct {
  mr_log_file_t* log_file;
  mtx_t* stdout_mutex;
  size_t* results_count;
  const char* thread_name;
} reducer_emit_arg_t;

/**
 * @brief Function called by the user's reducer to emit a result.
 */
static int reducer_emit(const char* token, const void* result,
                        size_t result_size, void* emit_arg) {
  reducer_emit_arg_t* arg = (reducer_emit_arg_t*)emit_arg;
  int res;

  if (mtx_lock(arg->stdout_mutex) != thrd_success) {
    mr_log_error(arg->log_file, "Reducer", arg->thread_name,
                 "Failed to lock stdout mutex");
    return -1;
  }

  res = send_result_to_main(arg->log_file, token, result, result_size);
  if (res == 0) {
    (*arg->results_count)++;
  }

  if (mtx_unlock(arg->stdout_mutex) != thrd_success) {
    mr_log_error(arg->log_file, "Reducer", arg->thread_name,
                 "Failed to unlock stdout mutex");
    return -1;
  }

  return res;
}

/**
 * @brief Worker thread function for the reducer process.
 */
static int reducer_worker(void* arg) {
  reducer_thread_arg_t* t_arg = (reducer_thread_arg_t*)arg;
  token_node_t* node;
  reducer_emit_arg_t e_arg = {.log_file = t_arg->log_file,
                             .stdout_mutex = t_arg->stdout_mutex,
                             .results_count = t_arg->results_count,
                             .thread_name = t_arg->thread_name};

  SYSCALL_EXIT_CHECK(mr_log_info(t_arg->log_file, "Reducer", t_arg->thread_name,
                           "Reducer worker thread started"),
                {});

  while (mr_queue_pop(t_arg->queue, &node) == 0) {
    if (t_arg->reducer(node->token, node->values, node->count, reducer_emit,
                       &e_arg, t_arg->user_arg) != 0) {
      mr_log_error(t_arg->log_file, "Reducer", t_arg->thread_name,
                   "User reducer function failed for token %s", node->token);
    }

    // Free resources for this token
    free(node->token);
    for (size_t i = 0; i < node->count; i++) {
      free((void*)node->values[i].data);
    }
    free(node->values);
    free(node);
  }

  SYSCALL_EXIT_CHECK(mr_log_info(t_arg->log_file, "Reducer", t_arg->thread_name,
                           "Reducer worker thread finished"),
                {});

  return 0;
}

/**
 * @brief Hash function for strings.
 */
static unsigned long hash_function(const char* str) {
  unsigned long hash = 5381;
  int c;
  while ((c = *str++))
    hash = ((hash << 5) + hash) + c;
  return hash % HASH_TABLE_SIZE;
}

/**
 * @brief Finds a token node in the hash table or creates a new one.
 */
static token_node_t* find_or_create_token(token_node_t** hash_table,
                                         const char* token) {
  unsigned long h = hash_function(token);
  token_node_t* node = hash_table[h];
  while (node) {
    if (strcmp(node->token, token) == 0)
      return node;
    node = node->next;
  }

  // Create new node
  node = malloc(sizeof(token_node_t));
  if (!node)
    return NULL;

  node->token = strdup(token);
  if (!node->token) {
    free(node);
    return NULL;
  }

  node->values = NULL;
  node->count = 0;
  node->capacity = 0;
  node->next = hash_table[h];
  hash_table[h] = node;

  return node;
}

/**
 * @brief Adds a value to a token node.
 */
static int add_value_to_token(token_node_t* node, void* value,
                             size_t value_size) {
  if (node->count == node->capacity) {
    size_t new_capacity = node->capacity == 0 ? 8 : node->capacity * 2;
    mr_value_t* new_values =
        realloc(node->values, new_capacity * sizeof(mr_value_t));
    if (!new_values)
      return -1;
    node->values = new_values;
    node->capacity = new_capacity;
  }

  node->values[node->count].data = value;
  node->values[node->count].size = value_size;
  node->count++;

  return 0;
}

/**
 * @brief Frees all resources associated with a token node.
 */
static void cleanup_token_node(token_node_t* node) {
  if (!node)
    return;
  free(node->token);
  for (size_t i = 0; i < node->count; i++) {
    free((void*)node->values[i].data);
  }
  free(node->values);
  free(node);
}

/**
 * @brief Frees all resources in the hash table.
 */
static void cleanup_hash_table(token_node_t** hash_table) {
  for (int i = 0; i < HASH_TABLE_SIZE; i++) {
    token_node_t* node = hash_table[i];
    while (node) {
      token_node_t* next = node->next;
      cleanup_token_node(node);
      node = next;
    }
    hash_table[i] = NULL;
  }
}

int reducer_process_main(mr_log_file_t* log_file, size_t reducer_threads,
                        size_t queue_size, mr_reducer_t reducer,
                        void* user_arg) {
  SYSCALL_EXIT_CHECK(mr_log_info(log_file, "Reducer", "Controller",
                           "Reducer process started with %zu threads and queue "
                           "size %zu",
                           reducer_threads, queue_size),
                {});

  token_node_t* hash_table[HASH_TABLE_SIZE] = {0};

  char* token;
  void* value;
  size_t value_size;
  int res;
  size_t pairs_received = 0;

  // Collection phase: receive pairs and group them by token
  while ((res = receive_pair_from_mapper(log_file, &token, &value,
                                         &value_size)) == 0) {
    pairs_received++;
    token_node_t* node = find_or_create_token(hash_table, token);
    if (!node) {
      mr_log_error(log_file, "Reducer", "Controller",
                   "Failed to find or create token node for token %s", token);
      free(token);
      free(value);
      res = -1;
      break;
    }

    if (add_value_to_token(node, value, value_size) != 0) {
      mr_log_error(log_file, "Reducer", "Controller",
                   "Failed to add value to token node for token %s", token);
      free(token);
      free(value);
      res = -1;
      break;
    }

    free(token); // find_or_create_token strdups it
  }

  if (res == -1) {
    mr_log_error(log_file, "Reducer", "Controller",
                 "Error during collection phase");
    cleanup_hash_table(hash_table);
    return EXIT_FAILURE;
  }

  // Processing phase: distribute tokens to worker threads
  mr_queue_t queue;
  SYSCALL_EXIT_CHECK(mr_queue_init(&queue, queue_size, sizeof(token_node_t*)),
                { cleanup_hash_table(hash_table); });

  mtx_t stdout_mutex;
  if (mtx_init(&stdout_mutex, mtx_plain) != thrd_success) {
    mr_log_error(log_file, "Reducer", "Controller",
                 "Failed to initialize stdout mutex");
    mr_queue_destroy(&queue);
    cleanup_hash_table(hash_table);
    return EXIT_FAILURE;
  }

  size_t results_count = 0;
  size_t tokens_count = 0;

  thrd_t* threads = malloc(sizeof(thrd_t) * reducer_threads);
  reducer_thread_arg_t* t_args =
      malloc(sizeof(reducer_thread_arg_t) * reducer_threads);

  if (threads == NULL || t_args == NULL) {
    mr_log_error(log_file, "Reducer", "Controller",
                 "Failed to allocate memory for reducer threads");
    free(threads);
    free(t_args);
    mtx_destroy(&stdout_mutex);
    mr_queue_destroy(&queue);
    cleanup_hash_table(hash_table);
    return EXIT_FAILURE;
  }

  for (size_t i = 0; i < reducer_threads; ++i) {
    t_args[i].queue = &queue;
    t_args[i].reducer = reducer;
    t_args[i].user_arg = user_arg;
    t_args[i].log_file = log_file;
    t_args[i].stdout_mutex = &stdout_mutex;
    t_args[i].results_count = &results_count;
    snprintf(t_args[i].thread_name, sizeof(t_args[i].thread_name), "Worker %zu",
             i);

    if (thrd_create(&threads[i], reducer_worker, &t_args[i]) != thrd_success) {
      mr_log_error(log_file, "Reducer", "Controller",
                   "Failed to create reducer worker thread %zu", i);
      mr_queue_close(&queue);
      for (size_t j = 0; j < i; ++j) {
        thrd_join(threads[j], NULL);
      }
      free(threads);
      free(t_args);
      mtx_destroy(&stdout_mutex);
      mr_queue_destroy(&queue);
      cleanup_hash_table(hash_table);
      return EXIT_FAILURE;
    }
  }

  // Push tokens from hash table into the queue
  for (int i = 0; i < HASH_TABLE_SIZE; i++) {
    token_node_t* node = hash_table[i];
    while (node) {
      tokens_count++;
      token_node_t* next = node->next;
      if (mr_queue_push(&queue, &node) != 0) {
        mr_log_error(log_file, "Reducer", "Controller",
                     "Failed to push token to queue");
        // In case of error, we still need to cleanup the rest of the hash table
        // and handle the threads.
        mr_queue_close(&queue);
        for (size_t j = 0; j < reducer_threads; ++j) {
          thrd_join(threads[j], NULL);
        }
        cleanup_token_node(node);
        node = next;
        while (node) {
          next = node->next;
          cleanup_token_node(node);
          node = next;
        }
        for (int k = i + 1; k < HASH_TABLE_SIZE; k++) {
          node = hash_table[k];
          while (node) {
            next = node->next;
            cleanup_token_node(node);
            node = next;
          }
        }
        free(threads);
        free(t_args);
        mtx_destroy(&stdout_mutex);
        mr_queue_destroy(&queue);
        return EXIT_FAILURE;
      }
      node = next;
    }
  }

  SYSCALL_EXIT_CHECK(mr_queue_close(&queue), {
    mr_log_error(log_file, "Reducer", "Controller", "Failed to close queue");
  });

  for (size_t i = 0; i < reducer_threads; ++i) {
    int thread_res;
    if (thrd_join(threads[i], &thread_res) != thrd_success) {
      mr_log_error(log_file, "Reducer", "Controller",
                   "Failed to join reducer worker thread %zu", i);
      return EXIT_FAILURE;
    }
    if (thread_res != EXIT_SUCCESS) {
      mr_log_warning(log_file, "Reducer", "Controller",
                     "Reducer worker thread %zu exited with error code %d", i,
                     thread_res);
    }
  }

  mtx_destroy(&stdout_mutex);
  mr_queue_destroy(&queue);
  free(threads);
  free(t_args);

  SYSCALL_EXIT_CHECK(mr_log_info(log_file, "Reducer", "Controller",
                           "Reducer process finished: %zu pairs received, %zu "
                           "tokens processed, %zu results produced",
                           pairs_received, tokens_count, results_count),
                {});

  return EXIT_SUCCESS;
}