#include "mapper.h"

#include <stdlib.h>
#include <string.h>
#include <threads.h>
#include <unistd.h>

#include "protocol.h"
#include "queue.h"
#include "utils.h"

/**
 * @brief Arguments passed to each mapper worker thread.
 */
typedef struct {
  mr_queue_t* queue;
  mr_mapper_t mapper;
  void* user_arg;
  mr_log_file_t* log_file;
  mtx_t* stdout_mutex;
  size_t* pairs_count;
  char thread_name[32];
} mapper_thread_arg_t;

/**
 * @brief Arguments passed to the mapper emit function.
 */
typedef struct {
  mr_log_file_t* log_file;
  mtx_t* stdout_mutex;
  size_t* pairs_count;
  const char* thread_name;
} mapper_emit_arg_t;

/**
 * @brief Function called by the user's mapper to emit a <token, value> pair.
  * @param token The token to emit.
  * @param value The value to emit.
  * @param value_size The size of the value in bytes.
  * @param emit_arg The argument passed to the emit function, containing necessary context.
  * @return 0 on success, -1 on failure.
 */
static int mapper_emit(const char* token, const void* value, size_t value_size,
                       void* emit_arg) {
  mapper_emit_arg_t* arg = (mapper_emit_arg_t*)emit_arg;
  int res;

  if (mtx_lock(arg->stdout_mutex) != thrd_success) {
    mr_log_error(arg->log_file, "Mapper", arg->thread_name,
                 "Failed to lock stdout mutex");
    return -1;
  }

  res = send_pair_to_reducer(arg->log_file, token, value, value_size);
  if (res == 0) {
    (*arg->pairs_count)++;
  }

  if (mtx_unlock(arg->stdout_mutex) != thrd_success) {
    mr_log_error(arg->log_file, "Mapper", arg->thread_name,
                 "Failed to unlock stdout mutex");
    return -1;
  }

  return res;
}

/**
 * @brief Worker thread function for the mapper process.
 */
static int mapper_worker(void* arg) {
  mapper_thread_arg_t* t_arg = (mapper_thread_arg_t*)arg;
  mr_file_line_t line;
  mapper_emit_arg_t e_arg = {.log_file = t_arg->log_file,
                             .stdout_mutex = t_arg->stdout_mutex,
                             .pairs_count = t_arg->pairs_count,
                             .thread_name = t_arg->thread_name};

  MRCALL_CHECK_CMD(mr_log_info(t_arg->log_file, "Mapper", t_arg->thread_name,
                           "Mapper worker thread started"), {
    return EXIT_FAILURE;
                           });

  while (mr_queue_pop(t_arg->queue, &line) == 0) {
    SYSCALL_CHECK_CMD(t_arg->mapper(&line, mapper_emit, &e_arg, t_arg->user_arg), {
      mr_log_error(t_arg->log_file, "Mapper", t_arg->thread_name,
                   "User mapper function failed for file %s, line %lu",
                   line.file_name, line.line_number);

      free((void*)line.line);
      free((void*)line.file_name);

      return EXIT_FAILURE;
    });

    // Free memory allocated by receive_line_from_main_fd and our strdup
    free((void*)line.line);
    free((void*)line.file_name);
  }

  MRCALL_CHECK_CMD(mr_log_info(t_arg->log_file, "Mapper", t_arg->thread_name,
                           "Mapper worker thread finished"), {
    return EXIT_FAILURE;
  });

  return 0;
}

int mapper_process_main(mr_log_file_t* log_file, size_t mapper_threads,
                        size_t queue_size, mr_mapper_t mapper, void* user_arg) {
  MRCALL_CHECK(mr_log_info(log_file, "Mapper", "Controller",
                           "Mapper process started with %zu threads and queue "
                           "size %zu",
                           mapper_threads, queue_size));

  mr_queue_t queue;
  if (mr_queue_init(&queue, queue_size, sizeof(mr_file_line_t)) != 0) {
    mr_log_error(log_file, "Mapper", "Controller",
                 "Failed to initialize mapper queue");
    return EXIT_FAILURE;
  }

  mtx_t stdout_mutex;
  if (mtx_init(&stdout_mutex, mtx_plain) != thrd_success) {
    mr_log_error(log_file, "Mapper", "Controller",
                 "Failed to initialize stdout mutex");
    mr_queue_destroy(&queue);
    return EXIT_FAILURE;
  }

  size_t pairs_count = 0;
  size_t lines_count = 0;

  thrd_t* threads = malloc(sizeof(thrd_t) * mapper_threads);
  mapper_thread_arg_t* t_args =
      malloc(sizeof(mapper_thread_arg_t) * mapper_threads);

  if (threads == NULL || t_args == NULL) {
    mr_log_error(log_file, "Mapper", "Controller",
                 "Failed to allocate memory for mapper threads");
    free(threads);
    free(t_args);
    mtx_destroy(&stdout_mutex);
    mr_queue_destroy(&queue);
    return EXIT_FAILURE;
  }

  for (size_t i = 0; i < mapper_threads; ++i) {
    t_args[i].queue = &queue;
    t_args[i].mapper = mapper;
    t_args[i].user_arg = user_arg;
    t_args[i].log_file = log_file;
    t_args[i].stdout_mutex = &stdout_mutex;
    t_args[i].pairs_count = &pairs_count;
    snprintf(t_args[i].thread_name, sizeof(t_args[i].thread_name), "Worker %zu",
             i);

    if (thrd_create(&threads[i], mapper_worker, &t_args[i]) != thrd_success) {
      mr_log_error(log_file, "Mapper", "Controller",
                   "Failed to create mapper worker thread %zu", i);
      // Clean up previously created threads and exit
      mr_queue_close(&queue);
      for (size_t j = 0; j < i; ++j) {
        thrd_join(threads[j], NULL);
      }
      free(threads);
      free(t_args);
      mtx_destroy(&stdout_mutex);
      mr_queue_destroy(&queue);
      return EXIT_FAILURE;
    }
  }

  mr_main_receiver_t receiver;
  mr_main_receiver_init(&receiver);

  mr_file_line_t line;
  int res;
  while ((res = receive_line_from_main(log_file, &receiver, &line)) == 0) {
    lines_count++;
    SYSCALL_CHECK_CMD(mr_queue_push(&queue, &line), {
      mr_log_error(log_file, "Mapper", "Controller",
                   "Failed to push line to queue");
      free((void*)line.line);
      free((void*)line.file_name);
      break;
    });
  }

  SYSCALL_CHECK_CMD(mr_queue_close(&queue), {
    mr_log_error(log_file, "Mapper", "Controller", "Failed to close queue");
    return EXIT_FAILURE;
  });

  for (size_t i = 0; i < mapper_threads; ++i) {
    int thread_res;

    if (thrd_join(threads[i], &thread_res) != thrd_success) {
      mr_log_error(log_file, "Mapper", "Controller", "Failed to join mapper worker thread %zu", i);
      return EXIT_FAILURE;
    };

    if (thread_res != EXIT_SUCCESS) {
      mr_log_warning(log_file, "Mapper", "Controller",
                     "Mapper worker thread %zu exited with error code %d", i, thread_res);
    }
  }

  mr_main_receiver_destroy(&receiver);
  mtx_destroy(&stdout_mutex);
  mr_queue_destroy(&queue);
  free(threads);
  free(t_args);

  SYSCALL_CHECK_CMD(res, {
    mr_log_error(log_file, "Mapper", "Controller",
                 "Error receiving lines from main process");
    return EXIT_FAILURE;
  });

  MRCALL_CHECK_CMD(mr_log_info(log_file, "Mapper", "Controller",
                           "Mapper process finished: %zu lines received, %zu "
                           "pairs produced",
                           lines_count, pairs_count), {
    return EXIT_FAILURE;
  });

  return EXIT_SUCCESS;
}