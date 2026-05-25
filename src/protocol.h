#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "log.h"

/**
 * @brief Send input data from the main process to the mapper process through a
 * pipe.
 * @param log_file Pointer to the log file for logging errors and information.
 * @param write_fd File descriptor for the write end of the pipe to the mapper
 * process.
 * @param input_path Path to the input file that contains the data to be
 * processed by the mapper.
 * @return 0 on success, -1 on failure.
 *
 * @pre - log_file not NULL
 * - input_path is not NULL
 */
int send_input_to_mapper(mr_log_file_t* log_file, int write_fd,
                         const char* input_path);

/** @brief Receive output data from the reducer process through a pipe and write
 * it to an output file.
 * @param log_file Pointer to the log file for logging errors and information.
 * @param read_fd File descriptor for the read end of the pipe from the reducer
 * process.
 * @param output_path Path to the output file where the data received from the
 * reducer will be written.
 * @return 0 on success, -1 on failure.
 *
 * @pre - log_file not NULL
 * - output_path is not NULL
 */
int receive_output_from_reducer(mr_log_file_t* log_file, int read_fd,
                                const char* output_path);

#endif /* PROTOCOL_H */
