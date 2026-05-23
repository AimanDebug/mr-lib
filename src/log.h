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

#include <config.h>

/**
 * @brief Internal helper function to initialize logging resources.
 *
 * Logs to the specified log file path, which may be created if it does not
 * exist.
 *
 * @param log_file_path Path to the log file to open for appending logs.
 *
 * @return 0 on success, -1 on error with errno set appropriately.
 *
 * @pre - log_file_path is not NULL.
 *
 * @note SYCALL_CHECK can be used to check the return value of this function and
 * handle errors appropriately.
 */
int mr_log_init(const char* log_file_path);

/**
 * @brief Internal helper function to write a log message.
 *
 * Logs messages in the format: <timestamp> <process_name>[pid]
 * <thread_name> <event>: <formatted message>
 *
 * @param level is the log level of the message.
 * @param pname is the name of the process logging the message.
 * @param tname is the name of the thread logging the message.
 * @param format Format string for the log message.
 * @param ... Additional arguments for the format string.
 *
 * @return 0 on success, -1 on error with errno set appropriately.
 *
 * @pre - format is not NULL.
 *
 * @note SYCALL_CHECK can be used to check the return value of this function and
 * handle errors appropriately.
 */
int mr_log(const char* level, const char* pname, const char* tname,
           const char* format, ...);

/**
 * @brief Internal helper function to clean up logging resources for the
 * MapReduce job.
 *
 * @return 0 on success, -1 on error with errno set appropriately.
 *
 * @note SYCALL_CHECK can be used to check the return value of this function and
 * handle errors appropriately.
 */
int mr_log_destroy(void);

#if MR_LOG_LEVEL <= MR_LOG_LEVEL_DEBUG
#define mr_log_debug(...) mr_log("debug", __VA_ARGS__)
#else
#define mr_log_debug(...)
#endif

#if MR_LOG_LEVEL <= MR_LOG_LEVEL_INFO
#define mr_log_info(...) mr_log("info", __VA_ARGS__)
#else
#define mr_log_info(...)
#endif

#if MR_LOG_LEVEL <= MR_LOG_LEVEL_WARNING
#define mr_log_warning(...) mr_log("warning", __VA_ARGS__)
#else
#define mr_log_warning(...)
#endif

#if MR_LOG_LEVEL <= MR_LOG_LEVEL_ERROR
#define mr_log_error(...) mr_log("error", __VA_ARGS__)
#else
#define mr_log_error(...)
#endif

#endif // LOG_H
