#ifndef MAPPER_H
#define MAPPER_H

#include "log.h"
#include <mr.h>

int mapper_process_main(mr_log_file_t* log_file, size_t mapper_threads,
                        size_t queue_size, mr_mapper_t mapper, void* user_arg);

#endif // MAPPER_H
