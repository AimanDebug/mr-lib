#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mr.h"

/**
 * @brief Mapper function for word count.
 * 
 * Splits each line by whitespace and emits a (token, 1)
 */
int word_count_mapper(const mr_file_line_t* line, mr_emit_pair_t emit,
                      void* emit_arg, void* user_arg) {
    (void)user_arg;
    char* line_copy = strdup(line->line);
    
    char* token = strtok(line_copy, " ");

    while (token) {
        emit(token, NULL, 0, emit_arg);
        token = strtok(NULL, " ");
    }

    free(line_copy);
    return 0;
}

/**
 * @brief Reducer function for word count.
 * 
 * Sums the counts for each token and emits (token, total_count)
 */
int word_count_reducer(const char* token, const mr_value_t* values,
                       size_t values_count, mr_emit_result_t emit,
                       void* emit_arg, void* user_arg) {
    (void)values;
    (void)user_arg;
    return emit(token, &values_count, sizeof(values_count), emit_arg);
}

int main(int argc, char** argv) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <input_path> <output_path>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char* input_path = argv[1];
    const char* output_path = argv[2];

    mr_t mr;
    mr_attr_t attr;

    if (mr_attr_init(&attr) == -1) {
        perror("mr_attr_init");
        return EXIT_FAILURE;
    }

    // Optional: customize attributes
    mr_attr_set_mapper_threads(&attr, 4);
    mr_attr_set_reducer_threads(&attr, 4);
    mr_attr_set_queue_size(&attr, 100);
    mr_attr_set_log_file(&attr, "word_count.log");

    if (mr_create(&mr, &attr, word_count_mapper, word_count_reducer, NULL) == -1) {
        perror("mr_create");
        mr_attr_destroy(&attr);
        return EXIT_FAILURE;
    }

    printf("Starting MapReduce on '%s' -> '%s'\\n", input_path, output_path);
    if (mr_start(mr, input_path, output_path) == -1) {
        perror("mr_start");
        mr_destroy(mr);
        mr_attr_destroy(&attr);
        return EXIT_FAILURE;
    }

    printf("MapReduce finished successfully.\\n");

    mr_destroy(mr);
    mr_attr_destroy(&attr);

    return EXIT_SUCCESS;
}
