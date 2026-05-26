#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mr.h"

/**
 * @brief Mapper function for word count.
 * 
 * Splits each line into alphanumeric tokens, converts them to lowercase,
 * and emits each token with a NULL value (as we only care about the count).
 */
int word_count_mapper(const mr_file_line_t* line, mr_emit_pair_t emit,
                      void* emit_arg, void* user_arg) {
    (void)user_arg;
    
    const char* start = line->line;
    const char* end = line->line + line->line_len;
    const char* curr = start;

    while (curr < end) {
        // Skip non-alphanumeric characters
        while (curr < end && !isalnum((unsigned char)*curr)) {
            curr++;
        }

        if (curr >= end) break;

        // Found the start of a token
        const char* token_start = curr;
        while (curr < end && isalnum((unsigned char)*curr)) {
            curr++;
        }
        size_t token_len = curr - token_start;

        // Copy and convert to lowercase
        char* token = malloc(token_len + 1);
        if (!token) return -1;

        for (size_t i = 0; i < token_len; i++) {
            token[i] = (char)tolower((unsigned char)token_start[i]);
        }
        token[token_len] = '\0';

        // Emit the token
        if (emit(token, NULL, 0, emit_arg) == -1) {
            free(token);
            return -1;
        }

        free(token);
    }

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

/**
 * @brief Reads the binary output file and prints the results to stdout.
 */
void print_results(const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) {
        perror("fopen output");
        return;
    }

    printf("\n--- Results ---\n");
    size_t token_len;
    while (fread(&token_len, sizeof(size_t), 1, f) == 1) {
        char* token = malloc(token_len + 1);
        if (!token) break;
        if (fread(token, 1, token_len, f) != token_len) {
            free(token);
            break;
        }
        token[token_len] = '\0';

        size_t result_size;
        if (fread(&result_size, sizeof(size_t), 1, f) != 1) {
            free(token);
            break;
        }

        if (result_size == sizeof(size_t)) {
            size_t count;
            if (fread(&count, sizeof(size_t), 1, f) != 1) {
                free(token);
                break;
            }
            printf("%s: %zu\n", token, count);
        } else {
            // Unexpected result size, skip it
            fseek(f, result_size, SEEK_CUR);
            printf("%s: [unknown data]\n", token);
        }
        free(token);
    }
    printf("----------------\n");

    fclose(f);
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

    printf("Starting MapReduce on '%s' -> '%s'\n", input_path, output_path);
    if (mr_start(mr, input_path, output_path) == -1) {
        perror("mr_start");
        mr_destroy(mr);
        mr_attr_destroy(&attr);
        return EXIT_FAILURE;
    }

    printf("MapReduce finished successfully.\n");

    print_results(output_path);

    mr_destroy(mr);
    mr_attr_destroy(&attr);

    return EXIT_SUCCESS;
}
