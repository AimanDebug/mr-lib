/**
 * @file libmr.c
 * @brief Implementation of the libmr library.
 * @author Adnaan Juma
 *
 * This file contains the implementation of the functions declared in libmr.h.
 */

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
