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

static int is_directory(mr_log_file_t* log_file, const char* path,
                        bool* out_result) {
  struct stat path_stat;

  SYSCALL_CHECK_CMD(stat(path, &path_stat), {
    mr_log_error(log_file, "Main", "Main", "Failed to stat path '%s'", path);
  });

  *out_result = S_ISDIR(path_stat.st_mode);

  return 0;
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

  file_name_header_t file_name_header = {.file_name_length = strlen(file_path)};

  SYSCALL_CHECK_CMD(
      write_n(write_fd, &file_name_header, sizeof(file_name_header)), {
        mr_log_error(log_file, "Main", "Main",
                     "Failed to send file name header for file '%s'",
                     file_path);
        fclose(file);
      });

  char* buffer = NULL;
  size_t buffer_size = 0;
  size_t line_length;

  while ((line_length = getline(&buffer, &buffer_size, file)) > 0) {
    line_header_t line_header = {
        .eof = false,
        .line_length = line_length - 1}; // Exclude the newline character

    SYSCALL_CHECK_CMD(write_n(write_fd, &line_header, sizeof(line_header)), {
      mr_log_error(log_file, "Main", "Main",
                   "Failed to send line header for file '%s'", file_path);
      free(buffer);
      fclose(file);
    });

    SYSCALL_CHECK_CMD(write_n(write_fd, buffer, line_length), {
      mr_log_error(log_file, "Main", "Main",
                   "Failed to send line data for file '%s'", file_path);
      free(buffer);
      fclose(file);
    });
  }

  free(buffer);

  if (ferror(file)) {
    mr_log_error(log_file, "Main", "Main",
                 "Error occurred while reading file '%s'", file_path);
    fclose(file);
    return -1;
  }

  // Send end of file header
  bool eof = true;

  SYSCALL_CHECK_CMD(write_n(write_fd, &eof, sizeof(eof)), {
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
    bool is_dir;

    MRCALL_CHECK_CMD(is_directory(log_file, namelist[i]->d_name, &is_dir), {
      mr_log_error(log_file, "Main", "Main",
                   "Failed to determine if entry '%s' is a directory",
                   namelist[i]);

      for (int j = i; j < files; j++) {
        free(namelist[j]);
      }

      free(namelist);
    });

    if (is_dir) {
      mr_log_warning(log_file, "Main", "Main",
                     "Skipping directory '%s' in input directory '%s'",
                     namelist[i]->d_name, dir_path);

      free(namelist[i]);
      continue;
    }

    MRCALL_CHECK_CMD(
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

  bool is_dir;

  MRCALL_CHECK(is_directory(log_file, input_path, &is_dir));

  MRCALL_CHECK(mr_log_info(log_file, "Main", "Main", "Input path '%s' is a %s",
                           input_path, is_dir ? "directory" : "file"));

  if (!is_dir)
    MRCALL_CHECK(send_file_to_mapper(log_file, write_fd, input_path));
  else
    MRCALL_CHECK(
        send_files_in_directory_to_mapper(log_file, write_fd, input_path));

  return 0;
}
