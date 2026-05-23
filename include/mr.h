/**
 * @file mr.h
 * @author Adnaan Juma
 * @brief Public interface for the MapReduce framework libmr.
 */

#ifndef MR_H
#define MR_H

#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Opaque handle for a MapReduce execution.
 */
typedef struct mr* mr_t;

/**
 * @brief Configuration attributes for the framework.
 */
typedef struct {
  /** Number of threads used in the mapper process. */
  size_t mapper_threads;
  /** Number of threads used in the reducer process. */
  size_t reducer_threads;

  /**
   * Maximum capacity of the internal queues used to coordinate
   * C11 threads in the mapper and reducer processes.
   * This does not represent the size of the pipes.
   */
  size_t queue_size;

  /** Name of the log file, or NULL to use the default name. */
  const char* log_file;
} mr_attr_t;

/**
 * @brief Logical line of a file, as seen by the mapper function.
 *
 * The file_name and line fields are pointers valid in the mapper process
 * only during the invocation of the mapper function.
 *
 * file_name and line are not necessarily null-terminated ('\0').
 * Their respective lengths are indicated by file_name_len and line_len.
 */
typedef struct {
  /** Name of the file the line comes from. */
  const char* file_name;
  /** Length of the file name in bytes. */
  size_t file_name_len;
  /** Line number within the file, starting from 1. */
  unsigned long line_number;
  /** Content of the line. */
  const char* line;
  /** Length of the line in bytes. */
  size_t line_len;
} mr_file_line_t;

/**
 * @brief Opaque value associated with a token.
 *
 * The framework does not interpret the content of data.
 * If size is 0, data can be NULL.
 */
typedef struct {
  /** Pointer to the opaque data. */
  const void* data;
  /** Size of the data in bytes. */
  size_t size;
} mr_value_t;

/**
 * @brief Function type used by the mapper to emit a <token, value> pair.
 *
 * @param token Must be a valid C string, null-terminated, composed only of
 * ASCII alphanumeric characters.
 * @param value Opaque sequence of bytes of length value_size.
 * @param value_size Size of the value in bytes. If 0, value can be NULL.
 * @param emit_arg Internal argument used by the framework.
 * @return 0 on success, -1 on error.
 */
typedef int (*mr_emit_pair_t)(const char* token, const void* value,
                              size_t value_size, void* emit_arg);

/**
 * @brief Function type used by the reducer to emit a final result.
 *
 * @param token Must be a valid C string. In the base project, it must match the
 * token received by the reducer.
 * @param result Opaque sequence of bytes of length result_size.
 * @param result_size Size of the result in bytes. If 0, result can be NULL.
 * @param emit_arg Internal argument used by the framework.
 * @return 0 on success, -1 on error.
 */
typedef int (*mr_emit_result_t)(const char* token, const void* result,
                                size_t result_size, void* emit_arg);

/**
 * @brief Mapper function type provided by the user program.
 *
 * Receives a logical line and can emit zero or more <token, value> pairs using
 * the emit function.
 *
 * @param line Pointer to the structure representing the logical line.
 * @param emit Function to call to emit a pair.
 * @param emit_arg Argument to pass to the emit function.
 * @param user_arg User-provided argument passed to mr_create.
 * @return 0 on success, -1 on error.
 */
typedef int (*mr_mapper_t)(const mr_file_line_t* line, mr_emit_pair_t emit,
                           void* emit_arg, void* user_arg);

/**
 * @brief Reducer function type provided by the user program.
 *
 * Receives a token and all values associated with it. Can emit zero or more
 * results using the emit function.
 *
 * @param token The token string.
 * @param values Array of opaque values associated with the token.
 * @param values_count Number of values in the values array.
 * @param emit Function to call to emit a result.
 * @param emit_arg Argument to pass to the emit function.
 * @param user_arg User-provided argument passed to mr_create.
 * @return 0 on success, -1 on error.
 */
typedef int (*mr_reducer_t)(const char* token, const mr_value_t* values,
                            size_t values_count, mr_emit_result_t emit,
                            void* emit_arg, void* user_arg);

/**
 * @brief Initializes framework attributes with default values.
 * @param attr Pointer to the attributes structure to initialize.
 * @return 0 on success, -1 on error.
 */
int mr_attr_init(mr_attr_t* attr);

/**
 * @brief Destroys framework attributes.
 * @param attr Pointer to the attributes structure to destroy.
 * @return 0 on success, -1 on error.
 */
int mr_attr_destroy(mr_attr_t* attr);

/**
 * @brief Sets the number of threads used in the mapper process.
 * @param attr Pointer to the attributes structure.
 * @param n Number of threads (must be at least 1).
 * @return 0 on success, -1 on error.
 */
int mr_attr_set_mapper_threads(mr_attr_t* attr, size_t n);

/**
 * @brief Sets the number of threads used in the reducer process.
 * @param attr Pointer to the attributes structure.
 * @param n Number of threads (must be at least 1).
 * @return 0 on success, -1 on error.
 */
int mr_attr_set_reducer_threads(mr_attr_t* attr, size_t n);

/**
 * @brief Sets the maximum capacity of internal queues.
 * @param attr Pointer to the attributes structure.
 * @param n Capacity (must be at least 1).
 * @return 0 on success, -1 on error.
 */
int mr_attr_set_queue_size(mr_attr_t* attr, size_t n);

/**
 * @brief Sets the path for the log file.
 * @param attr Pointer to the attributes structure.
 * @param path Path to the log file.
 * If NULL, resets to the default log file name.
 * @return 0 on success, -1 on error.
 */
int mr_attr_set_log_file(mr_attr_t* attr, const char* path);

/**
 * @brief Creates a new MapReduce execution instance.
 * @param mr Pointer to the handle to be initialized.
 * @param attr Configuration attributes.
 * @param mapper User-provided mapper function.
 * @param reducer User-provided reducer function.
 * @param user_arg Argument to pass to mapper and reducer functions.
 * @return 0 on success, -1 on error.
 */
int mr_create(mr_t* mr, const mr_attr_t* attr, mr_mapper_t mapper,
              mr_reducer_t reducer, void* user_arg);

/**
 * @brief Starts the MapReduce execution.
 *
 * This function is blocking and returns only when the execution is finished or
 * an error occurs.
 *
 * @param mr The MapReduce handle.
 * @param input_path Path to a single file or a directory of files.
 * @param output_path Path to the output file.
 * @return 0 on success, -1 on error.
 */
int mr_start(mr_t mr, const char* input_path, const char* output_path);

/**
 * @brief Releases all resources associated with a MapReduce instance.
 * @param mr The MapReduce handle, if NULL does nothing.
 * @return 0 on success, -1 on error.
 */
int mr_destroy(mr_t mr);

#endif /* MR_H */
