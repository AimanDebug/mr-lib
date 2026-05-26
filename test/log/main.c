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

static mr_log_file_t log_file;
static bool log_initialized = false;

void setUp(void) {
  remove(TEST_LOG_FILE);
  log_initialized = false;
  log_file.file = NULL;
  log_file.sem = SEM_FAILED;
}

void tearDown(void) {
  if (log_initialized) {
    mr_log_destroy(&log_file);
  }
  remove(TEST_LOG_FILE);
}

static void check_log_file_content(const char* expected_level, const char* pname,
                                   const char* tname, const char* message) {
  FILE* file = fopen(TEST_LOG_FILE, "r");
  TEST_ASSERT_NOT_NULL_MESSAGE(file, "Failed to open log file for checking");

  char expected[512];
  // Format: pname[pid] tname level: message
  sprintf(expected, "%s[%d] %s %s: %s\n", pname, getpid(), tname,
          expected_level, message);

  char buffer[512];
  if (!fgets(buffer, sizeof(buffer), file)) {
    fclose(file);
    TEST_FAIL_MESSAGE("Failed to read from log file");
  }

  // Check if expected string is in the buffer (ignoring potential timestamp)
  TEST_ASSERT_NOT_NULL_MESSAGE(strstr(buffer, expected), buffer);

  if (fgets(buffer, sizeof(buffer), file)) {
    fclose(file);
    TEST_FAIL_MESSAGE("Log file contains unexpected additional lines");
  }

  fclose(file);
}

void test_mr_log_init_success(void) {
  TEST_ASSERT_EQUAL_INT(0, mr_log_init(&log_file, TEST_LOG_FILE));
  log_initialized = true;
  TEST_ASSERT_NOT_NULL(log_file.file);
  TEST_ASSERT_NOT_EQUAL(SEM_FAILED, log_file.sem);
}

void test_mr_log_info_success(void) {
  TEST_ASSERT_EQUAL_INT(0, mr_log_init(&log_file, TEST_LOG_FILE));
  log_initialized = true;

  TEST_ASSERT_EQUAL_INT(0, mr_log_info(&log_file, "TestProc", "Thread0", TEST_STRING));
  
  // We need to close the file to ensure it's flushed if we were reading it from another process,
  // but here mr_log calls fflush. However, check_log_file_content opens it separately.
  
  check_log_file_content("info", "TestProc", "Thread0", TEST_STRING);
}

void test_mr_log_error_success(void) {
  TEST_ASSERT_EQUAL_INT(0, mr_log_init(&log_file, TEST_LOG_FILE));
  log_initialized = true;

  TEST_ASSERT_EQUAL_INT(0, mr_log_error(&log_file, "TestProc", "Thread0", "Error message"));
  
  check_log_file_content("error", "TestProc", "Thread0", "Error message");
}

void test_mr_log_multiple_lines(void) {
  TEST_ASSERT_EQUAL_INT(0, mr_log_init(&log_file, TEST_LOG_FILE));
  log_initialized = true;

  TEST_ASSERT_EQUAL_INT(0, mr_log_info(&log_file, "P1", "T1", "Msg1"));
  TEST_ASSERT_EQUAL_INT(0, mr_log_info(&log_file, "P1", "T1", "Msg2"));

  FILE* file = fopen(TEST_LOG_FILE, "r");
  TEST_ASSERT_NOT_NULL(file);

  char buffer[512];
  char expected[512];

  sprintf(expected, "P1[%d] T1 info: Msg1\n", getpid());
  TEST_ASSERT_NOT_NULL(fgets(buffer, sizeof(buffer), file));
  TEST_ASSERT_NOT_NULL_MESSAGE(strstr(buffer, expected), buffer);

  sprintf(expected, "P1[%d] T1 info: Msg2\n", getpid());
  TEST_ASSERT_NOT_NULL(fgets(buffer, sizeof(buffer), file));
  TEST_ASSERT_NOT_NULL_MESSAGE(strstr(buffer, expected), buffer);

  fclose(file);
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_mr_log_init_success);
  RUN_TEST(test_mr_log_info_success);
  RUN_TEST(test_mr_log_error_success);
  RUN_TEST(test_mr_log_multiple_lines);
  return UNITY_END();
}
