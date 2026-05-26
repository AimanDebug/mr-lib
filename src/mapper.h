#ifndef MAPPER_H
#define MAPPER_H

#include "log.h"
#include <mr.h>

/**
 * @brief Processes the main logic for the mapper.
 * @param log_file The log file to use for logging.
 * @param mapper_threads The number of mapper threads to use.
 * @param queue_size The size of the queue for each mapper thread.
 * @param mapper The mapper function to use.
 * @param user_arg The user argument to pass to the mapper function.
 * @return EXIT_SUCCESS on success, EXIT_FAILURE on failure.
 */
int mapper_process_main(mr_log_file_t* log_file, size_t mapper_threads,
                        size_t queue_size, mr_mapper_t mapper, void* user_arg);

#endif // MAPPER_H
