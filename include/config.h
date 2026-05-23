/**
 * @brief Configuration for the MapReduce framework.
 * @author Adnaan Juma
 */
#ifndef CONFIG_H
#define CONFIG_H

#include <assert.h>
#include <stddef.h>

#ifndef MR_DEFAULT_MAPPER_THREADS
#define MR_DEFAULT_MAPPER_THREADS 4
#endif
static_assert((size_t)MR_DEFAULT_MAPPER_THREADS > (size_t)0,
              "Default mapper threads must be at least 1");

#ifndef MR_DEFAULT_REDUCER_THREADS
#define MR_DEFAULT_REDUCER_THREADS 4 //< Default number of reducer threads
#endif
static_assert((size_t)MR_DEFAULT_REDUCER_THREADS > (size_t)0,
              "Default reducer threads must be at least 1");

#ifndef MR_DEFAULT_QUEUE_SIZE
#define MR_DEFAULT_QUEUE_SIZE 100 //< Default size of the task queue in elements
#endif
static_assert((size_t)MR_DEFAULT_QUEUE_SIZE > (size_t)0,
              "Default queue size must be at least 1");

#ifndef MR_DEFAULT_LOG_FILE
#define MR_DEFAULT_LOG_FILE "libmr.log" //< Default log file name
#endif

static_assert(sizeof(MR_DEFAULT_LOG_FILE) > (size_t)1,
              "Default log filename must be non-empty");

#endif // CONFIG_H
