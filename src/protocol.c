#include "protocol.h"

#include <dirent.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "utils.h"
#include <config.h>

ssize_t read_n(int fd, void* buffer, size_t n) {
  assert(fd >= 0);
  assert(buffer != NULL);

  size_t total_read = 0;
  char* buf_ptr = (char*)buffer;

  while (total_read < n) {
    ssize_t bytes_read;

    SYSCALL(bytes_read, read(fd, buf_ptr + total_read, n - total_read), {});

    if (bytes_read == 0) // EOF
      return total_read;

    total_read += bytes_read;
  }

  return total_read;
}

ssize_t write_n(int fd, const void* buffer, size_t n) {
  assert(fd >= 0);
  assert(buffer != NULL);

  size_t total_written = 0;
  const char* buf_ptr = (const char*)buffer;

  while (total_written < n) {
    ssize_t bytes_written;

    SYSCALL(bytes_written,
            write(fd, buf_ptr + total_written, n - total_written), {});

    total_written += bytes_written;
  }

  return total_written;
}

static int is_directory(mr_log_file_t* log_file, const char* path) {
  struct stat path_stat;

  SYSCALL_CHECK_CMD(stat(path, &path_stat), {
    mr_log_error(log_file, "Main", "Main", "Failed to stat path '%s'", path);
  });

  return S_ISDIR(path_stat.st_mode) ? 1 : 0;
}

static int send_file_to_mapper(mr_log_file_t* log_file, int write_fd,
                               const char* file_path) {
  assert(log_file != NULL);
  assert(write_fd >= 0);
  assert(file_path != NULL);

  MRCALL_CHECK(mr_log_info(log_file, "Main", "Main",
                           "Sending file '%s' to mapper", file_path));

  FILE* file = fopen(file_path, "r");

  if (file == NULL) {
    mr_log_error(log_file, "Main", "Main", "Failed to open file '%s'",
                 file_path);
    return -1;
  }

  size_t file_path_len = strlen(file_path);
  mr_file_name_header_t file_name_header = {.file_name_length = file_path_len};

  SYSCALL_CHECK_CMD(
      write_n(write_fd, &file_name_header, sizeof(file_name_header)), {
        mr_log_error(log_file, "Main", "Main",
                     "Failed to send file name header for file '%s'",
                     file_path);
        fclose(file);
      });

  SYSCALL_CHECK_CMD(write_n(write_fd, file_path, file_path_len), {
    mr_log_error(log_file, "Main", "Main",
                 "Failed to send file name string for file '%s'", file_path);
    fclose(file);
  });

  char* buffer = NULL;
  size_t buffer_size = 0;
  ssize_t read_bytes;

  while ((read_bytes = getline(&buffer, &buffer_size, file)) > 0) {
    size_t line_len = (size_t)read_bytes;
    if (buffer[line_len - 1] == '\n') {
      line_len--;
    }

    mr_line_header_t line_header = {.eof = false, .line_length = line_len};

    SYSCALL_CHECK_CMD(write_n(write_fd, &line_header, sizeof(line_header)), {
      mr_log_error(log_file, "Main", "Main",
                   "Failed to send line header for file '%s'", file_path);
      free(buffer);
      fclose(file);
    });

    if (line_len > 0) {
      SYSCALL_CHECK_CMD(write_n(write_fd, buffer, line_len), {
        mr_log_error(log_file, "Main", "Main",
                     "Failed to send line data for file '%s'", file_path);
        free(buffer);
        fclose(file);
      });
    }
  }

  free(buffer);

  if (ferror(file)) {
    mr_log_error(log_file, "Main", "Main",
                 "Error occurred while reading file '%s'", file_path);
    fclose(file);
    return -1;
  }

  // Send end of file header
  mr_line_header_t eof_header = {.eof = true, .line_length = 0};

  SYSCALL_CHECK_CMD(write_n(write_fd, &eof_header, sizeof(eof_header)), {
    mr_log_error(log_file, "Main", "Main", "Failed to send eof for file '%s'",
                 file_path);
    fclose(file);
  });

  if (fclose(file) == EOF) {
    mr_log_error(log_file, "Main", "Main", "Failed to close file '%s'",
                 file_path);
    return -1;
  }

  MRCALL_CHECK(mr_log_info(log_file, "Main", "Main",
                           "Finished sending file '%s' to mapper", file_path));

  return 0;
}

static int send_files_in_directory_to_mapper(mr_log_file_t* log_file,
                                             int write_fd,
                                             const char* dir_path) {
  MRCALL_CHECK(mr_log_info(
      log_file, "Main", "Main",
      "Scanning directory '%s' for files to send to mapper", dir_path));

  struct dirent** namelist;
  int files;

  files = scandir(dir_path, &namelist, NULL, alphasort);

  if (files < 0) {
    mr_log_error(log_file, "Main", "Main", "Failed to scan directory '%s'",
                 dir_path);
    return -1;
  }

  MRCALL_CHECK_CMD(mr_log_info(log_file, "Main", "Main",
                               "Found %d entries in directory '%s'", files,
                               dir_path),
                   {
                     for (int i = 0; i < files; i++) {
                       free(namelist[i]);
                     }
                     free(namelist);
                   });

  for (int i = 0; i < files; i++) {
    int is_dir;

    SYSCALL(is_dir, is_directory(log_file, namelist[i]->d_name), {
      mr_log_error(log_file, "Main", "Main",
                   "Failed to determine if entry '%s' is a directory",
                   namelist[i]->d_name);

      for (int j = i; j < files; j++) {
        free(namelist[j]);
      }

      free(namelist);
    });

    if (is_dir == 1) {
      mr_log_warning(log_file, "Main", "Main",
                     "Skipping directory '%s' in input directory '%s'",
                     namelist[i]->d_name, dir_path);

      free(namelist[i]);
      continue;
    }

    SYSCALL_CHECK_CMD(
        send_file_to_mapper(log_file, write_fd, namelist[i]->d_name), {
          mr_log_error(log_file, "Main", "Main",
                       "Failed to send file '%s' in directory '%s' to mapper",
                       namelist[i]->d_name, dir_path);

          for (int j = i; j < files; j++) {
            free(namelist[j]);
          }

          free(namelist);
        });

    free(namelist[i]);
  }

  free(namelist);

  MRCALL_CHECK(mr_log_info(log_file, "Main", "Main",
                           "Finished sending files in directory '%s' to mapper",
                           dir_path));

  return 0;
}

int send_input_to_mapper(mr_log_file_t* log_file, int write_fd,
                         const char* input_path) {
  assert(log_file != NULL);
  assert(write_fd >= 0);
  assert(input_path != NULL);

  int is_dir;

  SYSCALL(is_dir, is_directory(log_file, input_path), {});

  MRCALL_CHECK(mr_log_info(log_file, "Main", "Main", "Input path '%s' is a %s",
                           input_path, is_dir ? "directory" : "file"));

  if (!is_dir)
    SYSCALL_CHECK(send_file_to_mapper(log_file, write_fd, input_path));
  else
    SYSCALL_CHECK(
        send_files_in_directory_to_mapper(log_file, write_fd, input_path));

  MRCALL_CHECK(mr_log_info(log_file, "Main", "Main",
                           "Finished sending input from path '%s' to mapper",
                           input_path));

  return 0;
}

int receive_output_from_reducer(mr_log_file_t* log_file, int read_fd,
                                const char* output_path) {
  assert(log_file != NULL);
  assert(read_fd >= 0);
  assert(output_path != NULL);

  MRCALL_CHECK(mr_log_info(log_file, "Main", "Main",
                           "Receiving output from reducer and writing to '%s'",
                           output_path));

  FILE* out_file = fopen(output_path, "wb"); // Use binary mode for consistency
  if (out_file == NULL) {
    mr_log_error(log_file, "Main", "Main", "Failed to open output file '%s'",
                 output_path);
    return -1;
  }

  mr_result_header_t header;
  while (true) {
    ssize_t bytes_read = read_n(read_fd, &header, sizeof(header));
    if (bytes_read == 0) // EOF
      break;

    if (bytes_read != sizeof(header)) {
      mr_log_error(log_file, "Main", "Main",
                   "Failed to read result header from reducer");
      fclose(out_file);
      return -1;
    }

    // Write token length to file
    if (fwrite(&header.token_length, sizeof(header.token_length), 1,
               out_file) != 1) {
      mr_log_error(log_file, "Main", "Main",
                   "Failed to write token length to file");
      fclose(out_file);
      return -1;
    }

    // Allocate buffer for token and read it
    char* token = malloc(header.token_length);
    if (token == NULL && header.token_length > 0) {
      mr_log_error(log_file, "Main", "Main",
                   "Failed to allocate memory for token");
      fclose(out_file);
      return -1;
    }

    if (header.token_length > 0) {
      if (read_n(read_fd, token, header.token_length) !=
          (ssize_t)header.token_length) {
        mr_log_error(log_file, "Main", "Main",
                     "Failed to read token from reducer");
        free(token);
        fclose(out_file);
        return -1;
      }

      if (fwrite(token, 1, header.token_length, out_file) !=
          header.token_length) {
        mr_log_error(log_file, "Main", "Main", "Failed to write token to file");
        free(token);
        fclose(out_file);
        return -1;
      }
      free(token);
    }

    // Write result size to file
    if (fwrite(&header.result_size, sizeof(header.result_size), 1, out_file) !=
        1) {
      mr_log_error(log_file, "Main", "Main",
                   "Failed to write result size to file");
      fclose(out_file);
      return -1;
    }

    // Allocate buffer for result and read it
    void* result = malloc(header.result_size);
    if (result == NULL && header.result_size > 0) {
      mr_log_error(log_file, "Main", "Main",
                   "Failed to allocate memory for result");
      fclose(out_file);
      return -1;
    }

    if (header.result_size > 0) {
      if (read_n(read_fd, result, header.result_size) !=
          (ssize_t)header.result_size) {
        mr_log_error(log_file, "Main", "Main",
                     "Failed to read result from reducer");
        free(result);
        fclose(out_file);
        return -1;
      }

      if (fwrite(result, 1, header.result_size, out_file) !=
          header.result_size) {
        mr_log_error(log_file, "Main", "Main",
                     "Failed to write result to file");
        free(result);
        fclose(out_file);
        return -1;
      }
      free(result);
    }
  }

  if (fclose(out_file) == EOF) {
    mr_log_error(log_file, "Main", "Main", "Failed to close output file '%s'",
                 output_path);
    return -1;
  }

  MRCALL_CHECK(mr_log_info(log_file, "Main", "Main",
                           "Finished receiving output from reducer"));

  return 0;
}

int send_result_to_main_fd(mr_log_file_t* log_file, int write_fd,
                        const char* token, const void* result,
                        size_t result_size) {
  assert(log_file != NULL);
  assert(write_fd >= 0);
  assert(token != NULL);

  size_t token_len = strlen(token);
  mr_result_header_t header = {
      .token_length = token_len,
      .result_size = result_size,
  };

  SYSCALL_CHECK_CMD(write_n(write_fd, &header, sizeof(header)), {
    mr_log_error(log_file, "Reducer", "Controller",
                 "Failed to send result header to main process");
  });

  if (token_len > 0) {
    SYSCALL_CHECK_CMD(write_n(write_fd, token, token_len), {
      mr_log_error(log_file, "Reducer", "Controller",
                   "Failed to send token to main process");
    });
  }

  if (result_size > 0) {
    assert(result != NULL);
    SYSCALL_CHECK_CMD(write_n(write_fd, result, result_size), {
      mr_log_error(log_file, "Reducer", "Controller",
                   "Failed to send result data to main process");
    });
  }

  return 0;
}

int send_pair_to_reducer_fd(mr_log_file_t* log_file, int write_fd,
                         const char* token, const void* value,
                         size_t value_size) {
  assert(log_file != NULL);
  assert(write_fd >= 0);
  assert(token != NULL);

  size_t token_len = strlen(token);
  mr_pair_header_t header = {
      .token_length = token_len,
      .value_length = value_size,
  };

  SYSCALL_CHECK_CMD(write_n(write_fd, &header, sizeof(header)), {
    mr_log_error(log_file, "Mapper", "Controller",
                 "Failed to send pair header to reducer process");
  });

  if (token_len > 0) {
    SYSCALL_CHECK_CMD(write_n(write_fd, token, token_len), {
      mr_log_error(log_file, "Mapper", "Controller",
                   "Failed to send token to reducer process");
    });
  }

  if (value_size > 0) {
    assert(value != NULL);
    SYSCALL_CHECK_CMD(write_n(write_fd, value, value_size), {
      mr_log_error(log_file, "Mapper", "Controller",
                   "Failed to send value data to reducer process");
    });
  }

  return 0;
}

int receive_pair_from_mapper_fd(mr_log_file_t* log_file, int read_fd,
                             char** out_token, void** out_value,
                             size_t* out_value_size) {
  assert(log_file != NULL);
  assert(read_fd >= 0);
  assert(out_token != NULL);
  assert(out_value != NULL);
  assert(out_value_size != NULL);

  mr_pair_header_t header;
  ssize_t bytes_read;
  
  SYSCALL(bytes_read, read_n(read_fd, &header, sizeof(header)), {
    mr_log_error(log_file, "Mapper", "Controller",
                 "Failed to read pair header from mapper process");
    return -1;
  });

  if (bytes_read == 0)
    return 1; // EOF

  // Handle token
  *out_token = malloc(header.token_length + 1);

  // In case there was no data to allocate failure is fine
  if (*out_token == NULL && header.token_length > 0) {
    mr_log_error(log_file, "Reducer", "Controller",
                 "Failed to allocate memory for token");
    return -1;
  }

  if (header.token_length > 0) {
    SYSCALL_CHECK_CMD(read_n(read_fd, *out_token, header.token_length), {
      mr_log_error(log_file, "Reducer", "Controller",
                   "Failed to read token from mapper process");
      free(*out_token);
      return -1;
    });
  }

  // If you can insert a null terminator
  if (*out_token != NULL) {
    (*out_token)[header.token_length] = '\0';
  }

  // Handle value
  *out_value = malloc(header.value_length);
  if (*out_value == NULL && header.value_length > 0) {
    mr_log_error(log_file, "Reducer", "Controller",
                 "Failed to allocate memory for value");
    free(*out_token);
    return -1;
  }

  if (header.value_length > 0) {
    SYSCALL_CHECK_CMD(read_n(read_fd, *out_value, header.value_length), {
      mr_log_error(log_file, "Reducer", "Controller",
                   "Failed to read value data from mapper process");
      free(*out_token);
      free(*out_value);
      return -1;
    });
  }

  *out_value_size = header.value_length;

  return 0;
}

void mr_main_receiver_init(mr_main_receiver_t* receiver) {
  assert(receiver != NULL);
  receiver->current_file_name = NULL;
  receiver->current_file_name_len = 0;
  receiver->current_line_number = 0;
  receiver->file_finished = true;
}

void mr_main_receiver_destroy(mr_main_receiver_t* receiver) {
  if (receiver != NULL) {
    free(receiver->current_file_name);
    receiver->current_file_name = NULL;
  }
}

int receive_line_from_main_fd(mr_log_file_t* log_file, int read_fd,
                           mr_main_receiver_t* receiver,
                           mr_file_line_t* out_line) {
  assert(log_file != NULL);
  assert(read_fd >= 0);
  assert(receiver != NULL);
  assert(out_line != NULL);

  while (true) {
    if (receiver->file_finished) {
      mr_file_name_header_t fn_header;
      ssize_t bytes_read;
      
      SYSCALL(bytes_read, read_n(read_fd, &fn_header, sizeof(fn_header)), {
        mr_log_error(log_file, "Mapper", "Controller",
                     "Failed to read file name header from main process");
        return -1;
      });

      free(receiver->current_file_name);

      if (bytes_read == 0) {
        return 1; // Pipe EOF
      }

      receiver->current_file_name = malloc(fn_header.file_name_length + 1);
      if (receiver->current_file_name == NULL) {
        mr_log_error(log_file, "Mapper", "Controller",
                     "Failed to allocate memory for file name");
        return -1;
      }

      SYSCALL(bytes_read, read_n(read_fd, receiver->current_file_name,
                 fn_header.file_name_length), {
        mr_log_error(log_file, "Mapper", "Controller",
                     "Failed to read file name from main process");
        free(receiver->current_file_name);
        receiver->current_file_name = NULL;
        return -1;
      });

      receiver->current_file_name[fn_header.file_name_length] = '\0';
      receiver->current_file_name_len = fn_header.file_name_length;
      receiver->current_line_number = 1;
      receiver->file_finished = false;
    }

    mr_line_header_t l_header;
    ssize_t bytes_read;
    
    SYSCALL(bytes_read, read_n(read_fd, &l_header, sizeof(l_header)), {
      mr_log_error(log_file, "Mapper", "Controller",
                   "Failed to read line header from main process");
      return -1;
    });

    assert(bytes_read != 0); // Unexpected EOF in the middle of a file

    if (l_header.eof) {
      receiver->file_finished = true;
      continue; // Move to next file or pipe EOF
    }

    // make a copy of the file name for the line struct, because receiver might change it when the next file starts
    char* file_name_copy = malloc(receiver->current_file_name_len + 1);
    if (file_name_copy == NULL) {
      mr_log_error(log_file, "Mapper", "Controller",
                    "Failed to allocate memory for line file name");
      return -1;
    }
    memcpy(file_name_copy, receiver->current_file_name, receiver->current_file_name_len + 1);

    out_line->file_name = file_name_copy;
    out_line->file_name_len = receiver->current_file_name_len;
    out_line->line_number = receiver->current_line_number++;
    out_line->line_len = l_header.line_length;

    char* line_content = malloc(l_header.line_length + 1);
    if (line_content == NULL) {
      mr_log_error(log_file, "Mapper", "Controller",
                   "Failed to allocate memory for line content");
      return -1;
    }

    if (l_header.line_length > 0) {
      SYSCALL_CHECK_CMD(read_n(read_fd, line_content, l_header.line_length), {
        mr_log_error(log_file, "Mapper", "Controller",
                     "Failed to read line data from main process");
        free(line_content);
        return -1;
      });
    }
    line_content[l_header.line_length] = '\0';
    out_line->line = line_content;

    return 0;
  }
}

int receive_line_from_main(mr_log_file_t* log_file, mr_main_receiver_t* receiver, mr_file_line_t* out_line) {
  return receive_line_from_main_fd(log_file, STDIN_FILENO, receiver, out_line);
}

int send_pair_to_reducer(mr_log_file_t* log_file, const char* token, const void* value, size_t value_size) {
  return send_pair_to_reducer_fd(log_file, STDOUT_FILENO, token, value, value_size);
}

int receive_pair_from_mapper(mr_log_file_t* log_file, char** out_token, void** out_value, size_t* out_value_size) {
  return receive_pair_from_mapper_fd(log_file, STDIN_FILENO, out_token, out_value, out_value_size);
}

int send_result_to_main(mr_log_file_t* log_file, const char* token, const void* result, size_t result_size) {
  return send_result_to_main_fd(log_file, STDOUT_FILENO, token, result, result_size);
}
