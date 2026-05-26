#include <unity.h>
#include <mr.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define TEST_INPUT "test_core_input.txt"
#define TEST_OUTPUT "test_core_output.bin"
#define TEST_LOG "test_core.log"

void setUp(void) {
    remove(TEST_INPUT);
    remove(TEST_OUTPUT);
    remove(TEST_LOG);
}

void tearDown(void) {
    remove(TEST_INPUT);
    remove(TEST_OUTPUT);
    remove(TEST_LOG);
}

// Mapper: emits <word, 1>
static int test_mapper(const mr_file_line_t* line, mr_emit_pair_t emit,
                       void* emit_arg, void* user_arg) {
    (void)user_arg;
    char* line_copy = strndup(line->line, line->line_len);
    char* token = strtok(line_copy, " \t\n");
    int one = 1;
    while (token != NULL) {
        if (emit(token, &one, sizeof(int), emit_arg) == -1) {
            free(line_copy);
            return -1;
        }
        token = strtok(NULL, " \t\n");
    }
    free(line_copy);
    return 0;
}

// Reducer: sums the counts
static int test_reducer(const char* token, const mr_value_t* values,
                        size_t values_count, mr_emit_result_t emit,
                        void* emit_arg, void* user_arg) {
    (void)user_arg;
    int sum = 0;
    for (size_t i = 0; i < values_count; i++) {
        sum += *(int*)values[i].data;
    }
    return emit(token, &sum, sizeof(int), emit_arg);
}

void test_full_mapreduce_job(void) {
    // 1. Prepare input file
    FILE* f = fopen(TEST_INPUT, "w");
    TEST_ASSERT_NOT_NULL(f);
    fprintf(f, "apple banana apple\n");
    fprintf(f, "cherry banana\n");
    fprintf(f, "apple\n");
    fclose(f);

    // 2. Configure MR
    mr_t mr;
    mr_attr_t attr;
    TEST_ASSERT_EQUAL_INT(0, mr_attr_init(&attr));
    mr_attr_set_log_file(&attr, TEST_LOG);
    mr_attr_set_mapper_threads(&attr, 2);
    mr_attr_set_reducer_threads(&attr, 2);

    TEST_ASSERT_EQUAL_INT(0, mr_create(&mr, &attr, test_mapper, test_reducer, NULL));

    // 3. Start MR
    TEST_ASSERT_EQUAL_INT(0, mr_start(mr, TEST_INPUT, TEST_OUTPUT));

    // 4. Verify output
    // Expected: apple: 3, banana: 2, cherry: 1
    FILE* out = fopen(TEST_OUTPUT, "rb");
    TEST_ASSERT_NOT_NULL(out);

    int apple_count = 0;
    int banana_count = 0;
    int cherry_count = 0;

    size_t token_len;
    while (fread(&token_len, sizeof(size_t), 1, out) == 1) {
        char* token = malloc(token_len + 1);
        fread(token, 1, token_len, out);
        token[token_len] = '\0';

        size_t result_size;
        fread(&result_size, sizeof(size_t), 1, out);
        TEST_ASSERT_EQUAL(sizeof(int), result_size);

        int count;
        fread(&count, sizeof(int), 1, out);

        if (strcmp(token, "apple") == 0) apple_count = count;
        else if (strcmp(token, "banana") == 0) banana_count = count;
        else if (strcmp(token, "cherry") == 0) cherry_count = count;

        free(token);
    }
    fclose(out);

    TEST_ASSERT_EQUAL_INT(3, apple_count);
    TEST_ASSERT_EQUAL_INT(2, banana_count);
    TEST_ASSERT_EQUAL_INT(1, cherry_count);

    // 5. Cleanup
    TEST_ASSERT_EQUAL_INT(0, mr_destroy(mr));
    mr_attr_destroy(&attr);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_full_mapreduce_job);
    return UNITY_END();
}
