/**
 * @brief Configuration for the MapReduce framework.
 * @author Adnaan Juma
*/

#ifndef CONFIG_H
#define CONFIG_H

#define MR_DEFAULT_MAPPER_THREADS 4 //< Default number of mapper threads
#define MR_DEFAULT_REDUCER_THREADS 4 //< Default number of reducer threads
#define MR_DEFAULT_QUEUE_SIZE 100 //< Default size of the task queue in elements
#define MR_DEFAULT_LOG_FILE "libmr.log" //< Default log file name

#endif // CONFIG_H
