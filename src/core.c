#include "core.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "log.h"
#include "syscall.h"
#include <mr.h>

int mr_init(mr_t mr, const mr_attr_t* attr, mr_mapper_t mapper,
            mr_reducer_t reducer, void* user_arg) {
  assert(mr != NULL);
  assert(attr != NULL);
  assert(mr_attr_check(attr));
  assert(mapper != NULL);
  assert(reducer != NULL);

  mr->attr.mapper_threads = attr->mapper_threads;
  mr->attr.reducer_threads = attr->reducer_threads;
  mr->attr.queue_size = attr->queue_size;
  // Deep copy the log_file string
  size_t length = strlen(attr->log_file) + 1; // +1 for the null terminator
  if ((mr->attr.log_file = (char*)malloc(length)) == NULL) {
    // errno is set by malloc
    return -1;
  }
  memcpy((void*)mr->attr.log_file, (void*)attr->log_file, length);
  mr->mapper = mapper;
  mr->reducer = reducer;
  mr->user_arg = user_arg;

  return 0;
}

void mr_cleanup(mr_t mr) {
  assert(mr != NULL);

  // Free any resources allocated in mr_init
  free((void*)mr->attr.log_file);
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