/**
 * @file libmr.c
 * @brief Implementation of the libmr library.
 * @author Adnaan Juma
 *
 * This file contains the implementation of the functions declared in libmr.h.
 */

#include <assert.h>
#include <libmr.h>
#include <config.h>
#include <errno.h>

int mr_attr_init(mr_attr_t* attr) {
  if (attr == NULL) {
    errno = EINVAL; //< Invalid argument
    return -1;
  }

  attr->log_file = MR_DEFAULT_LOG_FILE;
  attr->mapper_threads = MR_DEFAULT_MAPPER_THREADS;
  attr->reducer_threads = MR_DEFAULT_REDUCER_THREADS;
  attr->queue_size = MR_DEFAULT_QUEUE_SIZE;

  return 0;
}

int mr_attr_set_mapper_threads(mr_attr_t* attr, size_t n) {
  if (attr == NULL || n < 1) {
    errno = EINVAL;
    return -1;
  }

  attr->mapper_threads = n;
  return 0;
}

int mr_attr_set_reducer_threads(mr_attr_t* attr, size_t n) {
  if (attr == NULL || n < 1) {
    errno = EINVAL;
    return -1;
  }

  attr->reducer_threads = n;
  return 0;
}

int mr_attr_set_queue_size(mr_attr_t* attr, size_t n) {
  if (attr == NULL || n < 1) {
    errno = EINVAL;
    return -1;
  }

  attr->queue_size = n;
  return 0;
}

int mr_attr_set_log_file(mr_attr_t* attr, const char* path) {
  if (attr == NULL) {
    errno = EINVAL;
    return -1;
  }

  // NOTE: path validity is not checked here, as it will be used later when opening the file.

  if (path == NULL)
    path = MR_DEFAULT_LOG_FILE;

  attr->log_file = path;
  return 0;
}

int mr_attr_destroy(mr_attr_t* attr) {
  if (attr == NULL) {
    errno = EINVAL;
    return -1;
  }

  // NOTE: no dynamic memory is allocated for the log_file, so we don't need to free it.
  // Provided for future extensibility.

  return 0;
}
