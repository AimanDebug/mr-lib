#include "reducer.h"

#include <stdlib.h>

#include "utils.h"

int reducer_process_main(mr_log_file_t* log_file, size_t reducer_threads,
                        size_t queue_size, mr_reducer_t reducer, void* user_arg) {
  SYSCALL_EXIT_CHECK(mr_log_info(log_file, "Reducer", "Controller",
              "Reducer process started with %zu threads and queue size %zu",
              reducer_threads, queue_size), {});

  (void)reducer;
  (void)user_arg;

  return 0;
}