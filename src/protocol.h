#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "log.h"
#include <stdbool.h>

typedef struct {
  size_t file_name_length; // Length of the file name string excluding \0
} file_name_header_t;

typedef struct {
  bool eof; //< Indicates if the previous line was the last line of the file
  size_t
      line_length; // Length of the line string excluding \n and \0 terminators
} line_header_t;

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

/**
 * @brief Receive output data from the reducer process through a pipe and write
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

/**
 * @brief Read exactly n bytes from a file descriptor into a buffer.
 * @param fd File descriptor to read from.
 * @param buffer Pointer to the buffer where the read data will be stored.
 * @param n Number of bytes to read.
 * @return Number of bytes read on success, -1 on failure.
 *
 * @pre - fd is a valid file descriptor
 * - buffer is not NULL
 */
ssize_t read_n(int fd, void* buffer, size_t n);

/**
 * @brief Write exactly n bytes from a buffer to a file descriptor.
 * @param fd File descriptor to write to.
 * @param buffer Pointer to the buffer that contains the data to be written.
 * @param n Number of bytes to write.
 * @return Number of bytes written on success, -1 on failure.
 *
 * @pre - fd is a valid file descriptor
 * - buffer is not NULL
 */
ssize_t write_n(int fd, const void* buffer, size_t n);

#endif /* PROTOCOL_H */
