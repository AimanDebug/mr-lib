#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "log.h"
#include <mr.h>
#include <stdbool.h>

typedef struct {
  size_t file_name_length; // Length of the file name string excluding \0
} mr_file_name_header_t;

typedef struct {
  bool eof; //< Indicates if the previous line was the last line of the file
  size_t
      line_length; // Length of the line string excluding \n and \0 terminators
} mr_line_header_t;

typedef struct {
  size_t token_length; // Length of the token string excluding \0
  size_t value_length; // Length of the value in bytes
} mr_pair_header_t;

typedef struct {
  size_t token_length; // Length of the token string excluding \0
  size_t result_size;  // Size of the result in bytes
} mr_result_header_t;

typedef struct {
  char* current_file_name;
  size_t current_file_name_len;
  unsigned long current_line_number;
  bool file_finished;
} mr_main_receiver_t;

/**
 * @brief Initializes the main receiver state.
 * @param receiver Pointer to the receiver state to initialize.
 */
void mr_main_receiver_init(mr_main_receiver_t* receiver);

/**
 * @brief Destroys the main receiver state and frees associated memory.
 * @param receiver Pointer to the receiver state to destroy.
 */
void mr_main_receiver_destroy(mr_main_receiver_t* receiver);

/**
 * @brief Receive a line from the main process through a pipe.
 * @param log_file Pointer to the log file.
 * @param read_fd File descriptor for the read end of the pipe.
 * @param receiver Pointer to the main receiver state.
 * @param out_line Pointer to the structure where the received line will be
 * stored.
 * @return 0 on success, 1 on EOF, -1 on failure.
 *
 * @pre - log_file not NULL
 * - receiver not NULL
 * - out_line not NULL
 * @note The caller is responsible for freeing the memory allocated for
 * out_line->line and out_line->file_name by this function.
 */
int receive_line_from_main_fd(mr_log_file_t* log_file, int read_fd,
                              mr_main_receiver_t* receiver,
                              mr_file_line_t* out_line);

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
 * @brief Send a result from the reducer process to the main process.
 * @param log_file Pointer to the log file.
 * @param write_fd File descriptor for the write end of the pipe.
 * @param token The token string.
 * @param result Pointer to the result data.
 * @param result_size Size of the result data.
 * @return 0 on success, -1 on failure.
 *
 * @pre - log_file not NULL
 * - token not NULL
 */
int send_result_to_main_fd(mr_log_file_t* log_file, int write_fd,
                           const char* token, const void* result,
                           size_t result_size);

/**
 * @brief Send a <token, value> pair from the mapper process to the reducer
 * process.
 * @param log_file Pointer to the log file.
 * @param write_fd File descriptor for the write end of the pipe.
 * @param token The token string.
 * @param value Pointer to the opaque value data.
 * @param value_size Size of the value data.
 * @return 0 on success, -1 on failure.
 *
 * @pre - log_file not NULL
 * - token not NULL
 */
int send_pair_to_reducer_fd(mr_log_file_t* log_file, int write_fd,
                            const char* token, const void* value,
                            size_t value_size);

/**
 * @brief Receive a <token, value> pair from the mapper process.
 * @param log_file Pointer to the log file.
 * @param read_fd File descriptor for the read end of the pipe.
 * @param out_token Pointer to store the received token (must be freed by
 * caller).
 * @param out_value Pointer to store the received value data (must be freed by
 * caller).
 * @param out_value_size Pointer to store the size of the received value data.
 * @return 0 on success, 1 on EOF, -1 on failure.
 *
 * @pre - log_file not NULL
 * - out_token not NULL
 * - out_value not NULL
 * - out_value_size not NULL
 */
int receive_pair_from_mapper_fd(mr_log_file_t* log_file, int read_fd,
                                char** out_token, void** out_value,
                                size_t* out_value_size);

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

/**
 * @brief Receive a line from the main process through stdin.
 * @param log_file Pointer to the log file.
 * @param receiver Pointer to the main receiver state.
 * @param out_line Pointer to the structure where the received line will be
 * stored.
 * @return 0 on success, 1 on EOF, -1 on failure.
 *
 * @pre - log_file not NULL
 * - receiver not NULL
 * - out_line not NULL
 * @note This is a wrapper around receive_line_from_main_fd that uses
 * STDIN_FILENO as the file descriptor. The caller is responsible for freeing
 * the memory allocated for out_line->line and out_line->file_name by this
 * function.
 */
int receive_line_from_main(mr_log_file_t* log_file,
                           mr_main_receiver_t* receiver,
                           mr_file_line_t* out_line);

/**
 * @brief Send a <token, value> pair from the mapper process to the reducer
 * process through stdout.
 * @param log_file Pointer to the log file.
 * @param token The token string.
 * @param value Pointer to the opaque value data.
 * @param value_size Size of the value data.
 * @return 0 on success, -1 on failure.
 *
 * @pre - log_file not NULL
 * - token not NULL
 * @note This function is a wrapper around send_pair_to_reducer_fd that uses
 * STDOUT_FILENO as the file descriptor.
 */
int send_pair_to_reducer(mr_log_file_t* log_file, const char* token,
                         const void* value, size_t value_size);

/**
 * @brief Receive a <token, value> pair from the mapper process through stdin.
 * @param log_file Pointer to the log file.
 * @param out_token Pointer to store the received token (must be freed by
 * caller).
 * @param out_value Pointer to store the received value data (must be freed by
 * caller).
 * @param out_value_size Pointer to store the size of the received value data.
 * @return 0 on success, 1 on EOF, -1 on failure.
 *
 * @pre - log_file not NULL
 * - out_token not NULL
 * - out_value not NULL
 * - out_value_size not NULL
 */
int receive_pair_from_mapper(mr_log_file_t* log_file, char** out_token,
                             void** out_value, size_t* out_value_size);

/**
 * @brief Send a result from the reducer process to the main process through
 * stdout.
 * @param log_file Pointer to the log file.
 * @param token The token string.
 * @param result Pointer to the result data.
 * @param result_size Size of the result data.
 * @return 0 on success, -1 on failure.
 *
 * @pre - log_file not NULL
 * - token not NULL
 */
int send_result_to_main(mr_log_file_t* log_file, const char* token,
                        const void* result, size_t result_size);

#endif /* PROTOCOL_H */
