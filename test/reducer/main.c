#include <unity.h>
#include <threads.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "reducer.h"
#include "protocol.h"
#include "log.h"

#define TEST_LOG_FILE "test_reducer.log"
static mr_log_file_t log_file;

void setUp(void) {
    mr_log_init(&log_file, TEST_LOG_FILE);
}

void tearDown(void) {
    mr_log_destroy(&log_file);
    remove(TEST_LOG_FILE);
}

// Mock reducer function
static int mock_reducer(const char* token, const mr_value_t* values,
                        size_t values_count, mr_emit_result_t emit,
                        void* emit_arg, void* user_arg) {
    (void)user_arg;
    int sum = 0;
    for (size_t i = 0; i < values_count; i++) {
        sum += *(int*)values[i].data;
    }
    return emit(token, &sum, sizeof(int), emit_arg);
}

void test_reducer_process_main(void) {
    int in_pipe[2];
    int out_pipe[2];
    TEST_ASSERT_EQUAL(0, pipe(in_pipe));
    TEST_ASSERT_EQUAL(0, pipe(out_pipe));

    // Save original stdin/stdout
    int orig_stdin = dup(STDIN_FILENO);
    int orig_stdout = dup(STDOUT_FILENO);

    // Redirect stdin/stdout to our pipes
    dup2(in_pipe[0], STDIN_FILENO);
    dup2(out_pipe[1], STDOUT_FILENO);

    // We need to write to in_pipe[1] and read from out_pipe[0]
    // But reducer_process_main is blocking. We should run it in a separate thread or process.
    // Let's use a thread for simplicity if reducer_process_main is thread-safe (it should be).
    
    // Write data to in_pipe[1]
    int val1 = 10;
    int val2 = 20;
    send_pair_to_reducer_fd(&log_file, in_pipe[1], "token1", &val1, sizeof(int));
    send_pair_to_reducer_fd(&log_file, in_pipe[1], "token1", &val2, sizeof(int));
    send_pair_to_reducer_fd(&log_file, in_pipe[1], "token2", &val1, sizeof(int));
    close(in_pipe[1]); // Send EOF

    // Call reducer_process_main
    int exit_code = reducer_process_main(&log_file, 2, 10, mock_reducer, NULL);
    TEST_ASSERT_EQUAL(EXIT_SUCCESS, exit_code);

    // Close write end of out_pipe so we can read EOF
    close(out_pipe[1]);

    // Restore stdin/stdout
    dup2(orig_stdin, STDIN_FILENO);
    dup2(orig_stdout, STDOUT_FILENO);
    close(orig_stdin);
    close(orig_stdout);

    // Check results from out_pipe[0]
    // We expect token1 -> 30, token2 -> 10
    mr_result_header_t header;
    bool found_token1 = false;
    bool found_token2 = false;

    while (read_n(out_pipe[0], &header, sizeof(header)) == sizeof(header)) {
        char* token = malloc(header.token_length + 1);
        read_n(out_pipe[0], token, header.token_length);
        token[header.token_length] = '\0';

        int result;
        TEST_ASSERT_EQUAL(sizeof(int), header.result_size);
        read_n(out_pipe[0], &result, sizeof(int));

        if (strcmp(token, "token1") == 0) {
            TEST_ASSERT_EQUAL(30, result);
            found_token1 = true;
        } else if (strcmp(token, "token2") == 0) {
            TEST_ASSERT_EQUAL(10, result);
            found_token2 = true;
        }
        free(token);
    }

    TEST_ASSERT_TRUE(found_token1);
    TEST_ASSERT_TRUE(found_token2);

    close(in_pipe[0]);
    close(out_pipe[0]);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_reducer_process_main);
    return UNITY_END();
}
