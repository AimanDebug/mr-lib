#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <unity.h>

#include "protocol.h"
#include "log.h"

#define TEST_LOG_FILE "test_protocol.log"

static mr_log_file_t log_file;

void setUp(void) {
    mr_log_init(&log_file, TEST_LOG_FILE);
}

void tearDown(void) {
    mr_log_destroy(&log_file);
    remove(TEST_LOG_FILE);
}

void test_read_write_n(void) {
    int pipefd[2];
    TEST_ASSERT_EQUAL(0, pipe(pipefd));

    const char* message = "Hello, Protocol!";
    size_t len = strlen(message) + 1;

    ssize_t written = write_n(pipefd[1], message, len);
    TEST_ASSERT_EQUAL(len, written);

    char buffer[64];
    ssize_t read_bytes = read_n(pipefd[0], buffer, len);
    TEST_ASSERT_EQUAL(len, read_bytes);
    TEST_ASSERT_EQUAL_STRING(message, buffer);

    close(pipefd[0]);
    close(pipefd[1]);
}

void test_pair_communication(void) {
    int pipefd[2];
    TEST_ASSERT_EQUAL(0, pipe(pipefd));

    const char* token = "test_token";
    const char* value = "test_value";
    size_t value_size = strlen(value) + 1;

    TEST_ASSERT_EQUAL(0, send_pair_to_reducer_fd(&log_file, pipefd[1], token, value, value_size));

    char* received_token = NULL;
    void* received_value = NULL;
    size_t received_value_size = 0;

    TEST_ASSERT_EQUAL(0, receive_pair_from_mapper_fd(&log_file, pipefd[0], &received_token, &received_value, &received_value_size));

    TEST_ASSERT_EQUAL_STRING(token, received_token);
    TEST_ASSERT_EQUAL(value_size, received_value_size);
    TEST_ASSERT_EQUAL_MEMORY(value, received_value, value_size);

    free(received_token);
    free(received_value);

    // Test EOF
    close(pipefd[1]);
    TEST_ASSERT_EQUAL(1, receive_pair_from_mapper_fd(&log_file, pipefd[0], &received_token, &received_value, &received_value_size));

    close(pipefd[0]);
}

void test_result_communication(void) {
    int pipefd[2];
    TEST_ASSERT_EQUAL(0, pipe(pipefd));

    const char* token = "result_token";
    const char* result_data = "result_data";
    size_t result_size = strlen(result_data) + 1;

    TEST_ASSERT_EQUAL(0, send_result_to_main_fd(&log_file, pipefd[1], token, result_data, result_size));
    close(pipefd[1]);

    const char* output_path = "test_output.bin";
    TEST_ASSERT_EQUAL(0, receive_output_from_reducer(&log_file, pipefd[0], output_path));
    close(pipefd[0]);

    // Verify output file content
    FILE* f = fopen(output_path, "rb");
    TEST_ASSERT_NOT_NULL(f);

    size_t read_token_len;
    TEST_ASSERT_EQUAL(1, fread(&read_token_len, sizeof(size_t), 1, f));
    TEST_ASSERT_EQUAL(strlen(token), read_token_len);

    char* read_token = malloc(read_token_len + 1);
    TEST_ASSERT_EQUAL(read_token_len, fread(read_token, 1, read_token_len, f));
    read_token[read_token_len] = '\0';
    TEST_ASSERT_EQUAL_STRING(token, read_token);
    free(read_token);

    size_t read_result_size;
    TEST_ASSERT_EQUAL(1, fread(&read_result_size, sizeof(size_t), 1, f));
    TEST_ASSERT_EQUAL(result_size, read_result_size);

    char* read_result = malloc(read_result_size);
    TEST_ASSERT_EQUAL(read_result_size, fread(read_result, 1, read_result_size, f));
    TEST_ASSERT_EQUAL_MEMORY(result_data, read_result, result_size);
    free(read_result);

    fclose(f);
    remove(output_path);
}

void test_line_communication(void) {
    int pipefd[2];
    TEST_ASSERT_EQUAL(0, pipe(pipefd));

    const char* test_file = "test_input.txt";
    FILE* f = fopen(test_file, "w");
    fprintf(f, "line1\nline2\nline3");
    fclose(f);

    mr_main_receiver_t receiver;
    mr_main_receiver_init(&receiver);

    // Manually simulate send_file_to_mapper
    const char* file_name = "test_input.txt";
    size_t fn_len = strlen(file_name);
    mr_file_name_header_t fn_header = { .file_name_length = fn_len };
    write_n(pipefd[1], &fn_header, sizeof(fn_header));
    write_n(pipefd[1], file_name, fn_len);

    const char* line1 = "line1";
    mr_line_header_t l1_header = { .eof = false, .line_length = strlen(line1) };
    write_n(pipefd[1], &l1_header, sizeof(l1_header));
    write_n(pipefd[1], line1, strlen(line1));

    mr_file_line_t out_line;
    TEST_ASSERT_EQUAL(0, receive_line_from_main_fd(&log_file, pipefd[0], &receiver, &out_line));
    TEST_ASSERT_EQUAL_STRING(file_name, out_line.file_name);
    TEST_ASSERT_EQUAL(1, out_line.line_number);
    TEST_ASSERT_EQUAL_STRING(line1, out_line.line);
    free((void*)out_line.line);

    const char* line2 = "line2";
    mr_line_header_t l2_header = { .eof = false, .line_length = strlen(line2) };
    write_n(pipefd[1], &l2_header, sizeof(l2_header));
    write_n(pipefd[1], line2, strlen(line2));

    TEST_ASSERT_EQUAL(0, receive_line_from_main_fd(&log_file, pipefd[0], &receiver, &out_line));
    TEST_ASSERT_EQUAL(2, out_line.line_number);
    TEST_ASSERT_EQUAL_STRING(line2, out_line.line);
    free((void*)out_line.line);

    mr_line_header_t eof_header = { .eof = true, .line_length = 0 };
    write_n(pipefd[1], &eof_header, sizeof(eof_header));

    close(pipefd[1]);
    TEST_ASSERT_EQUAL(1, receive_line_from_main_fd(&log_file, pipefd[0], &receiver, &out_line));

    mr_main_receiver_destroy(&receiver);
    close(pipefd[0]);
    remove(test_file);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_read_write_n);
    RUN_TEST(test_pair_communication);
    RUN_TEST(test_result_communication);
    RUN_TEST(test_line_communication);
    return UNITY_END();
}
