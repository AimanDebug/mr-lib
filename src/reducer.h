#ifndef REDUCER_H
#define REDUCER_H

#include "log.h"
#include <mr.h>

int reducer_process_main(mr_log_file_t* log_file, size_t reducer_threads,
                         size_t queue_size, mr_reducer_t reducer,
                         void* user_arg);

#endif // REDUCER_H
