#include "test_mr.h"
#include <unity.h>

#include <mr.h>
#include <config.h>

void test_mr_attr_init(void) {
    mr_attr_t attr;
    // Success case
    TEST_ASSERT_EQUAL_INT(0, mr_attr_init(&attr));
    TEST_ASSERT_EQUAL_size_t(MR_DEFAULT_MAPPER_THREADS, attr.mapper_threads);
    TEST_ASSERT_EQUAL_size_t(MR_DEFAULT_REDUCER_THREADS, attr.reducer_threads);
    TEST_ASSERT_EQUAL_size_t(MR_DEFAULT_QUEUE_SIZE, attr.queue_size);
    TEST_ASSERT_EQUAL_STRING(MR_DEFAULT_LOG_FILE, attr.log_file);

    // Fail case
    TEST_ASSERT_EQUAL_INT(-1, mr_attr_init(NULL));
}

void test_mr_attr_set_mapper_threads(void) {
    mr_attr_t attr;
    mr_attr_init(&attr);
    TEST_ASSERT_EQUAL_INT(0, mr_attr_set_mapper_threads(&attr, 8));
    TEST_ASSERT_EQUAL_size_t(8, attr.mapper_threads);
    TEST_ASSERT_EQUAL_INT(-1, mr_attr_set_mapper_threads(&attr, 0));
    TEST_ASSERT_EQUAL_INT(-1, mr_attr_set_mapper_threads(NULL, 8));
}

void test_mr_attr_set_reducer_threads(void) {
    mr_attr_t attr;
    mr_attr_init(&attr);
    TEST_ASSERT_EQUAL_INT(0, mr_attr_set_reducer_threads(&attr, 2));
    TEST_ASSERT_EQUAL_size_t(2, attr.reducer_threads);
    TEST_ASSERT_EQUAL_INT(-1, mr_attr_set_reducer_threads(&attr, 0));
    TEST_ASSERT_EQUAL_INT(-1, mr_attr_set_reducer_threads(NULL, 2));
}

void test_mr_attr_set_queue_size(void) {
    mr_attr_t attr;
    mr_attr_init(&attr);
    TEST_ASSERT_EQUAL_INT(0, mr_attr_set_queue_size(&attr, 200));
    TEST_ASSERT_EQUAL_size_t(200, attr.queue_size);
    TEST_ASSERT_EQUAL_INT(-1, mr_attr_set_queue_size(&attr, 0));
    TEST_ASSERT_EQUAL_INT(-1, mr_attr_set_queue_size(NULL, 200));
}

void test_mr_attr_set_log_file(void) {
    mr_attr_t attr;
    mr_attr_init(&attr);
    const char* log = "custom.log";

    // Test setting a custom log file
    TEST_ASSERT_EQUAL_INT(0, mr_attr_set_log_file(&attr, log));
    TEST_ASSERT_EQUAL_STRING(log, attr.log_file);

    // Test passing NULL attr returns error
    TEST_ASSERT_EQUAL_INT(-1, mr_attr_set_log_file(NULL, log));

    // Test setting log file to NULL resets to default
    mr_attr_set_log_file(&attr, NULL);
    TEST_ASSERT_EQUAL_STRING(MR_DEFAULT_LOG_FILE, attr.log_file);
}

void test_mr_attr_destroy(void) {
    mr_attr_t attr;
    mr_attr_init(&attr);
    // Success case
    TEST_ASSERT_EQUAL_INT(0, mr_attr_destroy(&attr));
    // Fail case
    TEST_ASSERT_EQUAL_INT(-1, mr_attr_destroy(NULL));
}

void test_mr(void) {
    RUN_TEST(test_mr_attr_init);
    RUN_TEST(test_mr_attr_set_mapper_threads);
    RUN_TEST(test_mr_attr_set_reducer_threads);
    RUN_TEST(test_mr_attr_set_queue_size);
    RUN_TEST(test_mr_attr_set_log_file);
    RUN_TEST(test_mr_attr_destroy);
}
