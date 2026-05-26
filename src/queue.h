/**
 * @file queue.h
 * @brief Bounded queue for thread coordination in the MapReduce framework.
 *
 * This queue implements a thread-safe, bounded LIFO buffer using C11 threads.
 * It is used to coordinate between producer and consumer threads in both
 * mapper and reducer processes.
 */

#ifndef QUEUE_H
#define QUEUE_H

#include <stdbool.h>
#include <stddef.h>
#include <threads.h>

/**
 * @brief Structure representing a bounded queue.
 */
typedef struct mr_queue {
  size_t capacity;       /**< Maximum capacity of the queue */
  size_t element_size;   /**< Size of each element in bytes */
  size_t count;          /**< Current number of elements in the queue */
  mtx_t mutex;           /**< Mutex for thread-safe access */
  cnd_t full;            /**< Condition variable: wait when queue is full */
  cnd_t empty;           /**< Condition variable: wait when queue is empty */
  bool closed;           /**< Flag indicating if the queue is closed */
  unsigned char* buffer; /**< Pointer to array to store queue elements */
} mr_queue_t;

/**
 * @brief Initializes a bounded queue with a fixed capacity.
 *
 * @param q The queue descriptor allocated by the caller.
 * @param capacity The maximum number of elements the queue can hold.
 * @param element_size The size of a single element in bytes.
 * @return 0 on success, -1 if an error occurs.
 */
int mr_queue_init(mr_queue_t* q, size_t capacity, size_t element_size);

/**
 * @brief Destroys the queue and frees all associated resources.
 *
 * @param q The queue to destroy.
 */
void mr_queue_destroy(mr_queue_t* q);

/**
 * @brief Pushes an element into the queue.
 *
 * The queue must not be closed when calling this function.
 * If the queue is full, this function blocks until space becomes available.
 *
 * @param q The queue.
 * @param data Pointer to the element to push. The element will be copied.
 * @return 0 on success, -1 if an error occurs.
 */
int mr_queue_push(mr_queue_t* q, const void* data);

/**
 * @brief Pops an element from the queue.
 *
 * If the queue is empty, this function blocks until an element is pushed
 * or the queue is closed and all existing elements have been popped.
 *
 * @param q The queue.
 * @param out_data Pointer to the buffer where the popped element will be
 * copied.
 * @return 0 on success, 1 if the queue is closed and empty, -1 on error.
 */
int mr_queue_pop(mr_queue_t* q, void* out_data);

/**
 * @brief Closes the queue, signaling that no more elements will be pushed.
 *
 * This will unblock any threads waiting to pop.
 *
 * @param q The queue.
 * @return 0 on success, -1 if an error occurs.
 */
int mr_queue_close(mr_queue_t* q);

#endif // QUEUE_H
