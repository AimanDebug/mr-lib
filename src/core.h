/**
 * @file core.h
 * @brief Internal header for MapReduce execution management.
 *
 * This header defines the internal structure and function prototypes for
 * managing MapReduce execution instances. It is not intended for public use and
 * should be included only within the implementation files of the MapReduce
 * library.
 *
 * @author Adnaan Juma
 */

#ifndef CORE_H
#define CORE_H

#include <mr.h>

/**
 * @brief Internal structure representing a MapReduce execution instance.
 */
struct mr {
  mr_attr_t attr; //< Configuration attributes for the execution
                  //  NOTE: attr.log_file should be owned.
  mr_mapper_t mapper;
  mr_reducer_t reducer;
  void* user_arg;
};

/**
 * @brief Initializes the MapReduce execution instance with the provided
 * attributes and functions.
 *
 * Deep copies the conf attrs and function pointers
 *
 * @param mr Pointer to MapReduce execution instance to initialize.
 * @param attr Configuration attributes for the execution.
 * @param mapper User-provided mapper function.
 * @param reducer User-provided reducer function.
 * @param user_arg Argument to pass to mapper and reducer functions.
 * @return 0 on success, -1 on error with errno set appropriately.
 *
 * @pre - mr is not NULL,
 *  - attr is not NULL and has been validated with mr_attr_check(),
 *  - mapper is not NULL,
 *  - reducer is not NULL.
 */
int mr_init(mr_t mr, const mr_attr_t* attr, mr_mapper_t mapper,
            mr_reducer_t reducer, void* user_arg);

/**
 * @brief Cleans up resources associated with the MapReduce execution instance.
 *
 * @param mr Pointer MapReduce execution instance to clean up.
 *
 * @pre mr is not NULL and has been initialized.
 */
void mr_cleanup(mr_t mr);

/**
 * @brief Runs the MapReduce job with the specified input and output paths.
 *
 * @param mr Pointer to initialized MapReduce execution instance.
 * @param input_path Path to the input data for the MapReduce job.
 * @param output_path Path where the output of the MapReduce job should be
 * written.
 *
 * @return 0 on success, -1 on error with errno set appropriately.
 *
 * @pre - mr is not NULL and has been initialized,
 *  - input_path is not NULL,
 *  - output_path is not NULL.
 */
int mr_run(mr_t mr, const char* input_path, const char* output_path);

/**
 * @brief Checks if the number of mapper threads is valid.
 */
bool mr_attr_check_mapper_threads(size_t n);

/**
 * @brief Checks if the number of reducer threads is valid.
 */
bool mr_attr_check_reducer_threads(size_t n);

/**
 * @brief Checks if the queue size is valid.
 */
bool mr_attr_check_queue_size(size_t n);

/**
 * @brief Checks if the log file path is valid.
 */
bool mr_attr_check_log_file(const char* path);

/**
 * @brief Validates all attributes in the mr_attr_t structure.
 */
bool mr_attr_check(const mr_attr_t* attr);

#endif // CORE_H
