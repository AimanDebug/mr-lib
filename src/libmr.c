/**
 * @file libmr.c
 * @brief Implementation of the libmr library.
 * @author Adnaan Juma
 *
 * This file contains the implementation of the functions declared in libmr.h.
 */

#include <assert.h>
#include <config.h>
#include <errno.h>
#include <libmr.h>

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

bool mr_attr_check_mapper_threads(size_t n) { return n >= 1; }
bool mr_attr_check_reducer_threads(size_t n) { return n >= 1; }
bool mr_attr_check_queue_size(size_t n) { return n >= 1; }
bool mr_attr_check_log_file(const char* path) {
  // NOTE: path validity is not checked here,
  // as it may be created later by the framework.
  (void)path;
  return true;
}
bool mr_attr_check(const mr_attr_t* attr) {
  assert(attr != NULL);

  return mr_attr_check_mapper_threads(attr->mapper_threads) &&
         mr_attr_check_reducer_threads(attr->reducer_threads) &&
         mr_attr_check_queue_size(attr->queue_size) &&
         mr_attr_check_log_file(attr->log_file);
}

int mr_attr_set_mapper_threads(mr_attr_t* attr, size_t n) {
  if (attr == NULL || !mr_attr_check_mapper_threads(n)) {
    errno = EINVAL;
    return -1;
  }

  attr->mapper_threads = n;
  return 0;
}

int mr_attr_set_reducer_threads(mr_attr_t* attr, size_t n) {
  if (attr == NULL || !mr_attr_check_reducer_threads(n)) {
    errno = EINVAL;
    return -1;
  }

  attr->reducer_threads = n;
  return 0;
}

int mr_attr_set_queue_size(mr_attr_t* attr, size_t n) {
  if (attr == NULL || !mr_attr_check_queue_size(n)) {
    errno = EINVAL;
    return -1;
  }

  attr->queue_size = n;
  return 0;
}

int mr_attr_set_log_file(mr_attr_t* attr, const char* path) {
  if (attr == NULL || !mr_attr_check_log_file(path)) {
    errno = EINVAL;
    return -1;
  }

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

  // NOTE: no dynamic memory is allocated for the log_file, 
  // so we don't need to free it. Provided for future extensibility.

  return 0;
}
