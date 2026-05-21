#include "test_libmr.h"
#include <unity.h>

#include <libmr.h>
#include <config.h>

void test_mr_attr_init_success(void) {
    mr_attr_t attr;
    TEST_ASSERT_EQUAL_INT(0, mr_attr_init(&attr));
    TEST_ASSERT_EQUAL_size_t(MR_DEFAULT_MAPPER_THREADS, attr.mapper_threads);
    TEST_ASSERT_EQUAL_size_t(MR_DEFAULT_REDUCER_THREADS, attr.reducer_threads);
    TEST_ASSERT_EQUAL_size_t(MR_DEFAULT_QUEUE_SIZE, attr.queue_size);
    TEST_ASSERT_EQUAL_STRING(MR_DEFAULT_LOG_FILE, attr.log_file);
}

void test_mr_attr_init_fail(void) {
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

void test_libmr(void) {
    RUN_TEST(test_mr_attr_init_success);
    RUN_TEST(test_mr_attr_init_fail);
    RUN_TEST(test_mr_attr_set_mapper_threads);
    RUN_TEST(test_mr_attr_set_reducer_threads);
}
