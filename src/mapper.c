#include "mapper.h"

#include "utils.h"

int mapper_process_main(mr_log_file_t* log_file, size_t mapper_threads,
                        size_t queue_size, mr_mapper_t mapper, void* user_arg) {
  MRCALL_CHECK(mr_log_info(log_file, "Mapper", "Controller",
              "Mapper process started with %zu threads and queue size %zu",
              mapper_threads, queue_size));

  (void)mapper;
  (void)user_arg;

  return 0;
}