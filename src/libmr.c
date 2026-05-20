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
