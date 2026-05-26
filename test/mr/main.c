#include <unity.h>
#include <mr.h>
#include <config.h>
#include <stdlib.h>

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

static int dummy_mapper(const mr_file_line_t* line, mr_emit_pair_t emit, void* emit_arg, void* user_arg) {
    (void)line; (void)emit; (void)emit_arg; (void)user_arg;
    return 0;
}

static int dummy_reducer(const char* token, const mr_value_t* values, size_t values_count, mr_emit_result_t emit, void* emit_arg, void* user_arg) {
    (void)token; (void)values; (void)values_count; (void)emit; (void)emit_arg; (void)user_arg;
    return 0;
}

void test_mr_create_destroy(void) {
    mr_t mr;
    mr_attr_t attr;
    mr_attr_init(&attr);

    TEST_ASSERT_EQUAL_INT(0, mr_create(&mr, &attr, dummy_mapper, dummy_reducer, NULL));
    TEST_ASSERT_NOT_NULL(mr);

    TEST_ASSERT_EQUAL_INT(0, mr_destroy(mr));
}

void test_mr_create_invalid(void) {
    mr_t mr;
    mr_attr_t attr;
    mr_attr_init(&attr);

    // NULL handle (well, mr is a pointer, mr_create takes &mr)
    // Actually mr_create(NULL, ...) would be bad.
    
    // NULL attr
    TEST_ASSERT_EQUAL_INT(-1, mr_create(&mr, NULL, dummy_mapper, dummy_reducer, NULL));
    
    // NULL mapper
    TEST_ASSERT_EQUAL_INT(-1, mr_create(&mr, &attr, NULL, dummy_reducer, NULL));
    
    // NULL reducer
    TEST_ASSERT_EQUAL_INT(-1, mr_create(&mr, &attr, dummy_mapper, NULL, NULL));
}

void setUp(void) {}
void tearDown(void) {}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_mr_attr_init);
    RUN_TEST(test_mr_attr_set_mapper_threads);
    RUN_TEST(test_mr_attr_set_reducer_threads);
    RUN_TEST(test_mr_attr_set_queue_size);
    RUN_TEST(test_mr_attr_set_log_file);
    RUN_TEST(test_mr_attr_destroy);
    RUN_TEST(test_mr_create_destroy);
    RUN_TEST(test_mr_create_invalid);
    return UNITY_END();
}
