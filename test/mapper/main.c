#include <unity.h>
#include <threads.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "mapper.h"
#include "protocol.h"
#include "log.h"

#define TEST_LOG_FILE "test_mapper.log"
static mr_log_file_t log_file;

void setUp(void) {
    mr_log_init(&log_file, TEST_LOG_FILE);
}

void tearDown(void) {
    mr_log_destroy(&log_file);
    remove(TEST_LOG_FILE);
}

// Mock mapper function
static int mock_mapper(const mr_file_line_t* line, mr_emit_pair_t emit,
                       void* emit_arg, void* user_arg) {
    (void)user_arg;
    // Emit <word, 1> for each word in the line
    char* line_copy = strndup(line->line, line->line_len);
    char* token = strtok(line_copy, " ");
    int one = 1;
    while (token != NULL) {
        emit(token, &one, sizeof(int), emit_arg);
        token = strtok(NULL, " ");
    }
    free(line_copy);
    return 0;
}

void test_mapper_process_main(void) {
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

    // Write data to in_pipe[1] (simulate main process sending lines)
    const char* file_name = "test.txt";
    size_t fn_len = strlen(file_name);
    mr_file_name_header_t fn_header = { .file_name_length = fn_len };
    write_n(in_pipe[1], &fn_header, sizeof(fn_header));
    write_n(in_pipe[1], file_name, fn_len);

    const char* line1 = "hello world";
    mr_line_header_t l1_header = { .eof = false, .line_length = strlen(line1) };
    write_n(in_pipe[1], &l1_header, sizeof(l1_header));
    write_n(in_pipe[1], line1, strlen(line1));

    mr_line_header_t eof_header = { .eof = true, .line_length = 0 };
    write_n(in_pipe[1], &eof_header, sizeof(eof_header));
    close(in_pipe[1]); // Send EOF

    // Call mapper_process_main
    int exit_code = mapper_process_main(&log_file, 2, 10, mock_mapper, NULL);
    TEST_ASSERT_EQUAL(EXIT_SUCCESS, exit_code);

    // Close write end of out_pipe
    close(out_pipe[1]);

    // Restore stdin/stdout
    dup2(orig_stdin, STDIN_FILENO);
    dup2(orig_stdout, STDOUT_FILENO);
    close(orig_stdin);
    close(orig_stdout);

    // Check results from out_pipe[0]
    mr_pair_header_t header;
    int hello_count = 0;
    int world_count = 0;

    while (read_n(out_pipe[0], &header, sizeof(header)) == sizeof(header)) {
        char* token = malloc(header.token_length + 1);
        read_n(out_pipe[0], token, header.token_length);
        token[header.token_length] = '\0';

        int value;
        TEST_ASSERT_EQUAL(sizeof(int), header.value_length);
        read_n(out_pipe[0], &value, sizeof(int));

        if (strcmp(token, "hello") == 0) {
            TEST_ASSERT_EQUAL(1, value);
            hello_count++;
        } else if (strcmp(token, "world") == 0) {
            TEST_ASSERT_EQUAL(1, value);
            world_count++;
        }
        free(token);
    }

    TEST_ASSERT_EQUAL(1, hello_count);
    TEST_ASSERT_EQUAL(1, world_count);

    close(in_pipe[0]);
    close(out_pipe[0]);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_mapper_process_main);
    return UNITY_END();
}
