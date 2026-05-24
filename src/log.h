/**
 * @file log.h
 * @brief Internal header for logging management.
 * @author Adnaan Juma
 *
 * This header defines the internal function prototypes for managing logging
 * resources for the MapReduce library. It is not intended for public use and
 * should be included only within the implementation files of the MapReduce
 * library.
 */

#ifndef LOG_H
#define LOG_H

#include <semaphore.h>
#include <stdio.h>

#include <config.h>

typedef struct mr_log_file {
  FILE* file;
  sem_t* sem;
} mr_log_file_t;

/**
 * @brief Internal helper function to initialize logging resources.
 *
 * Logs to the specified log file path, which may be created if it does not
 * exist.
 *
 * @param log Pointer to the logger to initialize.
 * @param log_file_path Path to the log file to open for appending logs.
 *
 * @return 0 on success, -1 on error with errno set appropriately.
 *
 * @pre - log is not NULL.
 * @pre - log_file_path is not NULL.
 *
 * @note SYCALL_CHECK can be used to check the return value of this function and
 * handle errors appropriately.
 */
int mr_log_init(mr_log_file_t* log, const char* log_file_path);

/**
 * @brief Internal helper function to write a log message.
 *
 * Logs messages in the format: <timestamp> <process_name>[pid]
 * <thread_name> <event>: <formatted message>
 *
 * @param log Pointer to the logger to write to.
 * @param level is the log level of the message.
 * @param pname is the name of the process logging the message.
 * @param tname is the name of the thread logging the message.
 * @param format Format string for the log message.
 * @param ... Additional arguments for the format string.
 *
 * @return 0 on success, -1 on error with errno set appropriately.
 *
 * @pre - log is not NULL and has been initialized.
 * @pre - format is not NULL.
 *
 * @note SYCALL_CHECK can be used to check the return value of this function and
 * handle errors appropriately.
 */
int mr_log(mr_log_file_t* log, const char* level, const char* pname,
           const char* tname, const char* format, ...);

/**
 * @brief Internal helper function to clean up logging resources for the
 * MapReduce job.
 *
 * @param log Pointer to the logger to destroy.
 *
 * @return 0 on success, -1 on error with errno set appropriately.
 *
 * @pre - log is not NULL.
 *
 * @note SYCALL_CHECK can be used to check the return value of this function and
 * handle errors appropriately.
 */
int mr_log_destroy(mr_log_file_t* log);

#if MR_LOG_LEVEL <= MR_LOG_LEVEL_DEBUG
#define mr_log_debug(log, ...) mr_log(log, "debug", __VA_ARGS__)
#else
#define mr_log_debug(log, ...)
#endif

#if MR_LOG_LEVEL <= MR_LOG_LEVEL_INFO
#define mr_log_info(log, ...) mr_log(log, "info", __VA_ARGS__)
#else
#define mr_log_info(log, ...)
#endif

#if MR_LOG_LEVEL <= MR_LOG_LEVEL_WARNING
#define mr_log_warning(log, ...) mr_log(log, "warning", __VA_ARGS__)
#else
#define mr_log_warning(log, ...)
#endif

#if MR_LOG_LEVEL <= MR_LOG_LEVEL_ERROR
#define mr_log_error(log, ...) mr_log(log, "error", __VA_ARGS__)
#else
#define mr_log_error(log, ...)
#endif

#endif // LOG_H
