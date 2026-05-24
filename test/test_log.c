#include "test_log.h"

#include <fcntl.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <unity.h>

#include <log.h>

#define TEST_LOG_FILE "test.log"
#define TEST_STRING "This is a test log message"
#define EXPECTED_STRING_FORMAT "Test Runner[%d] tt0 info: " TEST_STRING

static bool check_log_file(void) {
  // Check if the log file exists and is writable
  FILE* file;

  if (!(file = fopen(TEST_LOG_FILE, "r")))
    return false;

  // Check if the log file contains the expected string
  char expected[256];
  sprintf(expected, EXPECTED_STRING_FORMAT, getpid());

  char buffer[256];

  if (!fgets(buffer, sizeof(buffer), file)) {
    fclose(file);
    return false;
  }

  if (strcmp(buffer, expected)) {
    fclose(file);
    return false;
  }

  if (fgets(buffer, sizeof(buffer), file)) {
    fclose(file);
    return false;
  }

  if (!feof(file)) {
    fclose(file);
    return false;
  }

  fclose(file);

  return true;
}

void test_mr_log_success(void) {
  mr_log_file_t log_file;

  if (mr_log_init(&log_file, TEST_LOG_FILE) == -1) {
    mr_log_destroy(&log_file);
    remove(TEST_LOG_FILE);
    TEST_FAIL_MESSAGE("Failed to initialize log file");
  }

  // Log some messages
  if (mr_log_info(&log_file, "Test Runner", "tt0", TEST_STRING) == -1) {
    mr_log_destroy(&log_file);
    remove(TEST_LOG_FILE);
    TEST_FAIL_MESSAGE("Failed to log message");
  }

  if ((mr_log_destroy(&log_file) == -1)) {
    remove(TEST_LOG_FILE);
    TEST_FAIL_MESSAGE("Failed to destroy log file");
  }

  // Check the log file for the expected content
  if (!check_log_file()) {
    remove(TEST_LOG_FILE);
    TEST_FAIL_MESSAGE("Log file content does not match expected format");
  }

  remove(TEST_LOG_FILE);
}

void test_log(void) { RUN_TEST(test_mr_log_success); }
