#include "core.h"

#include <assert.h>
#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "log.h"
#include "mapper.h"
#include "protocol.h"
#include "reducer.h"
#include "utils.h"
#include <mr.h>

int mr_init(mr_t mr, const mr_attr_t* attr, mr_mapper_t mapper,
            mr_reducer_t reducer, void* user_arg) {
  assert(mr != NULL);
  assert(attr != NULL);
  assert(mr_attr_check(attr));
  assert(mapper != NULL);
  assert(reducer != NULL);

  // Set up attributes
  mr->attr.mapper_threads = attr->mapper_threads;
  mr->attr.reducer_threads = attr->reducer_threads;
  mr->attr.queue_size = attr->queue_size;
  
  // Deep copy the log_file string
  mr->attr.log_file = strdup(attr->log_file);
  if (mr->attr.log_file == NULL) {
    return -1;
  }

  mr->mapper = mapper;
  mr->reducer = reducer;
  mr->user_arg = user_arg;

  return 0;
}

void mr_cleanup(mr_t mr) {
  assert(mr != NULL);

  // Free any resources allocated in mr_init
  free((void*)mr->attr.log_file);
}

static int spawn_mapper_process(mr_log_file_t* log_file, pid_t* mapper_pid,
                                int main_to_mapper[2], int mapper_to_reducer[2],
                                int reducer_to_main[2], size_t mapper_threads,
                                size_t queue_size, mr_mapper_t mapper,
                                void* user_arg) {
  // Spawn mapper process
  SYSCALL_RET(*mapper_pid, fork(), {
    mr_log_error(log_file, "Main", "Main", "Failed to fork mapper process");

    for (uint8_t i = 0; i < 2; i++) {
      close(main_to_mapper[i]);
      close(mapper_to_reducer[i]);
    }
  });

  if (!(*mapper_pid)) {
    // Mapper process
    SYSCALL_RET_CHECK(mr_log_info(log_file, "Mapper", "Controller",
                              "Mapper process started."), {
                                for (uint8_t i = 0; i < 2; i++) {
                                  close(main_to_mapper[i]);
                                  close(mapper_to_reducer[i]);
                                  close(reducer_to_main[i]);
                                }
                              });

    // Close unused pipe ends
    SYSCALL_RET_CHECK(close(main_to_mapper[1]), {
      mr_log_error(log_file, "Mapper", "Controller",
                   "Failed to close unused write end of main_to_mapper pipe");

      close(main_to_mapper[0]);
      for (uint8_t i = 0; i < 2; i++) {
        close(mapper_to_reducer[i]);
        close(reducer_to_main[i]);
      }

      _exit(EXIT_FAILURE);
    });

    SYSCALL_RET_CHECK(close(mapper_to_reducer[0]), {
      mr_log_error(log_file, "Mapper", "Controller",
                   "Failed to close unused read end of mapper_to_reducer pipe");

      close(main_to_mapper[0]);
      close(mapper_to_reducer[1]);
      for (uint8_t i = 0; i < 2; i++)
        close(reducer_to_main[i]);

      _exit(EXIT_FAILURE);
    });

    SYSCALL_RET_CHECK(close(reducer_to_main[0]), {
      mr_log_error(log_file, "Mapper", "Controller",
                   "Failed to close unused read end of reducer_to_main pipe");

      close(main_to_mapper[0]);
      close(mapper_to_reducer[1]);
      close(reducer_to_main[1]);

      _exit(EXIT_FAILURE);
    });

    SYSCALL_RET_CHECK(close(reducer_to_main[1]), {
      mr_log_error(log_file, "Mapper", "Controller",
                   "Failed to close unused write end reducer_to_main pipe");

      close(main_to_mapper[0]);
      close(mapper_to_reducer[1]);

      _exit(EXIT_FAILURE);
    });

    // Redirect stdin
    SYSCALL_RET_CHECK(dup2(main_to_mapper[0], STDIN_FILENO), {
      mr_log_error(log_file, "Mapper", "Controller",
                   "Failed to redirect stdin for mapper process");

      close(main_to_mapper[0]);
      close(mapper_to_reducer[1]);

      _exit(EXIT_FAILURE);
    });

    SYSCALL_RET_CHECK(close(main_to_mapper[0]), {
      mr_log_error(log_file, "Mapper", "Controller",
                   "Failed to close read end of main_to_mapper pipe");

      close(mapper_to_reducer[1]);

      _exit(EXIT_FAILURE);
    });

    // Redirect stdout
    SYSCALL_RET_CHECK(dup2(mapper_to_reducer[1], STDOUT_FILENO), {
      mr_log_error(log_file, "Mapper", "Controller",
                   "Failed to redirect stdout for mapper process");

      close(mapper_to_reducer[1]);

      _exit(EXIT_FAILURE);
    });

    SYSCALL_RET_CHECK(close(mapper_to_reducer[1]), {
      mr_log_error(log_file, "Mapper", "Controller",
                   "Failed to close write end of mapper_to_reducer pipe");

      _exit(EXIT_FAILURE);
    });

    // Execute mapper process main function
    int exit_code = mapper_process_main(log_file, mapper_threads, queue_size,
                                        mapper, user_arg);
    _exit(exit_code);
  }

  return 0;
}

static int spawn_reducer_process(mr_log_file_t* log_file, pid_t* reducer_pid,
                                 int main_to_mapper[2],
                                 int mapper_to_reducer[2],
                                 int reducer_to_main[2], size_t reducer_threads,
                                 size_t queue_size, mr_reducer_t reducer,
                                 void* user_arg) {

  SYSCALL_RET(*reducer_pid, fork(), {
    mr_log_error(log_file, "Main", "Main", "Failed to fork reducer process");

    for (uint8_t i = 0; i < 2; i++) {
      close(mapper_to_reducer[i]);
      close(reducer_to_main[i]);
    }
  });

  if (!(*reducer_pid)) {
    // Reducer process
    SYSCALL_RET_CHECK(mr_log_info(log_file, "Reducer", "Controller",
                              "Reducer process started."), {
                                for (uint8_t i = 0; i < 2; i++) {
                                  close(main_to_mapper[i]);
                                  close(mapper_to_reducer[i]);
                                  close(reducer_to_main[i]);
                                }
                              });

    // Close unused pipe ends
    SYSCALL_RET_CHECK(close(main_to_mapper[0]), {
      mr_log_error(log_file, "Reducer", "Controller",
                   "Failed to close unused read end of main_to_mapper pipe");

      close(main_to_mapper[1]);

      for (uint8_t i = 0; i < 2; i++) {
        close(mapper_to_reducer[i]);
        close(reducer_to_main[i]);
      }

      _exit(EXIT_FAILURE);
    });

    SYSCALL_RET_CHECK(close(main_to_mapper[1]), {
      mr_log_error(log_file, "Reducer", "Controller",
                   "Failed to close unused write end of main_to_mapper pipe");

      for (uint8_t i = 0; i < 2; i++) {
        close(mapper_to_reducer[i]);
        close(reducer_to_main[i]);
      }

      _exit(EXIT_FAILURE);
    });

    SYSCALL_RET_CHECK(close(mapper_to_reducer[1]), {
      mr_log_error(
          log_file, "Reducer", "Controller",
          "Failed to close unused write end of mapper_to_reducer pipe");

      close(mapper_to_reducer[0]);
      for (uint8_t i = 0; i < 2; i++)
        close(reducer_to_main[i]);

      _exit(EXIT_FAILURE);
    });

    SYSCALL_RET_CHECK(close(reducer_to_main[0]), {
      mr_log_error(log_file, "Reducer", "Controller",
                   "Failed to close unused read end of reducer_to_main pipe");

      close(mapper_to_reducer[0]);
      close(reducer_to_main[1]);

      _exit(EXIT_FAILURE);
    });

    // Redirect stdin and stdout
    SYSCALL_RET_CHECK(dup2(mapper_to_reducer[0], STDIN_FILENO), {
      mr_log_error(log_file, "Reducer", "Controller",
                   "Failed to redirect stdin for reducer process");

      close(mapper_to_reducer[0]);
      close(reducer_to_main[1]);

      _exit(EXIT_FAILURE);
    });

    SYSCALL_RET_CHECK(close(mapper_to_reducer[0]), {
      mr_log_error(log_file, "Reducer", "Controller",
                   "Failed to close read end of mapper_to_reducer pipe");

      close(reducer_to_main[1]);

      _exit(EXIT_FAILURE);
    });

    SYSCALL_RET_CHECK(dup2(reducer_to_main[1], STDOUT_FILENO), {
      mr_log_error(log_file, "Reducer", "Controller",
                   "Failed to redirect stdout for reducer process");

      close(reducer_to_main[1]);

      _exit(EXIT_FAILURE);
    });

    SYSCALL_RET_CHECK(close(reducer_to_main[1]), {
      mr_log_error(log_file, "Reducer", "Controller",
                   "Failed to close write end of reducer_to_reducer pipe");

      _exit(EXIT_FAILURE);
    });

    int exit_code = reducer_process_main(log_file, reducer_threads, queue_size,
                                         reducer, user_arg);
    _exit(exit_code);
  }

  return 0;
}

static int wait_for_process(mr_log_file_t* log_file, const char* process_name,
                            pid_t pid) {
  int status;

  SYSCALL_RET_CHECK(waitpid(pid, &status, 0), {
    mr_log_error(log_file, "Main", "Main", "Failed to wait for %s process",
                 process_name);
  });

  if (WIFEXITED(status)) {
    int exit_code = WEXITSTATUS(status);
    SYSCALL_RET_CHECK(mr_log_info(log_file, "Main", "Main",
                             "%s process exited with code %d", process_name,
                             exit_code), {});
  } else if (WIFSIGNALED(status)) {
    int signal_num = WTERMSIG(status);
    SYSCALL_RET_CHECK(mr_log_warning(log_file, "Main", "Main",
                                "%s process was terminated by signal %d",
                                process_name, signal_num), {});
  } else {
    SYSCALL_RET_CHECK(mr_log_warning(log_file, "Main", "Main",
                                "%s process terminated abnormally",
                                process_name), {});
  }

  return 0;
}

static int spawn_processes(mr_t mr, const char* input_path,
                           const char* output_path, mr_log_file_t* log_file) {
  SYSCALL_RET_CHECK(mr_log_info(log_file, "Main", "Main",
                            "Spawning mapper and reducer processes..."), {});

  pid_t mapper_pid, reducer_pid;
  int main_to_mapper[2], mapper_to_reducer[2], reducer_to_main[2];

  // Open pipes
  SYSCALL_RET_CHECK(pipe(main_to_mapper), {
    mr_log_error(log_file, "Main", "Main",
                 "Failed to create pipe for main to mapper communication");
  });

  SYSCALL_RET_CHECK(pipe(mapper_to_reducer), {
    mr_log_error(log_file, "Main", "Main",
                 "Failed to create pipe for mapper to reducer communication");

    for (uint8_t i = 0; i < 2; i++)
      close(main_to_mapper[i]);
  });

  SYSCALL_RET_CHECK(pipe(reducer_to_main), {
    mr_log_error(log_file, "Main", "Main",
                 "Failed to create pipe for reducer to main communication");

    for (uint8_t i = 0; i < 2; i++) {
      close(main_to_mapper[i]);
      close(mapper_to_reducer[i]);
    }
  });

  // Spawn mapper
  SYSCALL_RET_CHECK(spawn_mapper_process(log_file, &mapper_pid, main_to_mapper,
                                         mapper_to_reducer, reducer_to_main,
                                         mr->attr.mapper_threads,
                                         mr->attr.queue_size, mr->mapper,
                                         mr->user_arg),
                    {
                      mr_log_error(log_file, "Main", "Main",
                                   "Failed to spawn mapper process");

                      for (uint8_t i = 0; i < 2; i++) {
                        close(main_to_mapper[i]);
                        close(mapper_to_reducer[i]);
                        close(reducer_to_main[i]);
                      }
                    });

  // Spawn reducer
  SYSCALL_RET_CHECK(
      spawn_reducer_process(log_file, &reducer_pid, main_to_mapper,
                            mapper_to_reducer, reducer_to_main,
                            mr->attr.reducer_threads, mr->attr.queue_size,
                            mr->reducer, mr->user_arg),
      {
        mr_log_error(log_file, "Main", "Main",
                     "Failed to spawn reducer process");

        kill(mapper_pid, SIGTERM);
        waitpid(mapper_pid, NULL, 0);

        close(main_to_mapper[1]);

        for (uint8_t i = 0; i < 2; i++) {
          close(mapper_to_reducer[i]);
          close(reducer_to_main[i]);
        }
      });

  // Close unused pipe ends in main process
  SYSCALL_RET_CHECK(close(main_to_mapper[0]), {
    kill(mapper_pid, SIGTERM);
    waitpid(mapper_pid, NULL, 0);

    close(main_to_mapper[1]);

    for (uint8_t i = 0; i < 2; i++) {
      close(mapper_to_reducer[i]);
      close(reducer_to_main[i]);
    }
  });

  SYSCALL_RET_CHECK(close(mapper_to_reducer[0]), {
    kill(mapper_pid, SIGTERM);
    kill(reducer_pid, SIGTERM);
    waitpid(mapper_pid, NULL, 0);
    waitpid(reducer_pid, NULL, 0);

    close(main_to_mapper[1]);

    close(mapper_to_reducer[1]);

    for (uint8_t j = 0; j < 2; j++) {
      close(reducer_to_main[j]);
    }
  });

  SYSCALL_RET_CHECK(close(mapper_to_reducer[1]), {
    kill(mapper_pid, SIGTERM);
    kill(reducer_pid, SIGTERM);
    waitpid(mapper_pid, NULL, 0);
    waitpid(reducer_pid, NULL, 0);

    close(main_to_mapper[1]);

    for (uint8_t j = 0; j < 2; j++) {
      close(reducer_to_main[j]);
    }
  });

  SYSCALL_RET_CHECK(close(reducer_to_main[1]), {
    kill(mapper_pid, SIGTERM);
    kill(reducer_pid, SIGTERM);
    waitpid(mapper_pid, NULL, 0);
    waitpid(reducer_pid, NULL, 0);

    close(main_to_mapper[1]);

    for (uint8_t i = 0; i < 2; i++) {
      close(mapper_to_reducer[i]);
      close(reducer_to_main[i]);
    }
  });

  SYSCALL_RET_CHECK(mr_log_info(log_file, "Main", "Main",
                               "Successfully spawned mapper (PID: %d) and "
                               "reducer (PID: %d) processes",
                               mapper_pid, reducer_pid),
                   {
                     kill(mapper_pid, SIGTERM);
                     kill(reducer_pid, SIGTERM);
                     waitpid(mapper_pid, NULL, 0);
                     waitpid(reducer_pid, NULL, 0);

                     close(main_to_mapper[1]);
                     close(reducer_to_main[0]);
                   });

  SYSCALL_RET_CHECK(
      send_input_to_mapper(log_file, main_to_mapper[1], input_path), {
        mr_log_error(log_file, "Main", "Main",
                     "Failed to send input to mapper process");
        kill(mapper_pid, SIGTERM);
        kill(reducer_pid, SIGTERM);
        waitpid(mapper_pid, NULL, 0);
        waitpid(reducer_pid, NULL, 0);

        close(main_to_mapper[1]);
        close(reducer_to_main[0]);
      });

  SYSCALL_RET_CHECK(close(main_to_mapper[1]), {
    mr_log_error(log_file, "Main", "Main",
                 "Failed to close write end of main_to_mapper pipe");

    kill(mapper_pid, SIGTERM);
    kill(reducer_pid, SIGTERM);
    waitpid(mapper_pid, NULL, 0);
    waitpid(reducer_pid, NULL, 0);

    close(reducer_to_main[0]);
  });

  SYSCALL_RET_CHECK(
      receive_output_from_reducer(log_file, reducer_to_main[0], output_path), {
        mr_log_error(log_file, "Main", "Main",
                     "Failed to receive output from reducer process");

        kill(mapper_pid, SIGTERM);
        kill(reducer_pid, SIGTERM);
        waitpid(mapper_pid, NULL, 0);
        waitpid(reducer_pid, NULL, 0);

        close(reducer_to_main[0]);
      });

  SYSCALL_RET_CHECK(close(reducer_to_main[0]), {
    mr_log_error(log_file, "Main", "Main",
                 "Failed to close read end of reducer_to_main pipe");

    kill(mapper_pid, SIGTERM);
    kill(reducer_pid, SIGTERM);
    waitpid(mapper_pid, NULL, 0);
    waitpid(reducer_pid, NULL, 0);
  });

  // Wait for the processes to finish
  SYSCALL_RET_CHECK(wait_for_process(log_file, "Mapper", mapper_pid), {
    mr_log_error(log_file, "Main", "Main",
                 "Failed to wait for mapper process to finish");

    kill(reducer_pid, SIGTERM);
    waitpid(reducer_pid, NULL, 0);
  });

  SYSCALL_RET_CHECK(wait_for_process(log_file, "Reducer", reducer_pid), {
    mr_log_error(log_file, "Main", "Main",
                 "Failed to wait for reducer process to finish");
  });

  return 0;
}

int mr_run(mr_t mr, const char* input_path, const char* output_path) {
  assert(mr != NULL);
  assert(input_path != NULL);
  assert(output_path != NULL);

  mr_log_file_t log_file;

  SYSCALL_RET_CHECK(mr_log_init(&log_file, mr->attr.log_file), {});

  SYSCALL_RET_CHECK(
      mr_log_info(&log_file, "Main", "",
                  "Starting MapReduce job with input: %s and output: %s",
                  input_path, output_path),
      mr_log_destroy(&log_file));

  SYSCALL_RET_CHECK(spawn_processes(mr, input_path, output_path, &log_file),
                   mr_log_destroy(&log_file));

  SYSCALL_RET_CHECK(mr_log_destroy(&log_file), {});

  return 0;
}

bool mr_attr_check_mapper_threads(size_t n) { return n >= 1; }
bool mr_attr_check_reducer_threads(size_t n) { return n >= 1; }
bool mr_attr_check_queue_size(size_t n) { return n >= 1; }
bool mr_attr_check_log_file(const char* path) {
  // NOTE: path validity is not checked here,
  // as it may be created later by the framework.
  (void)path;
  return true;
}
bool mr_attr_check(const mr_attr_t* attr) {
  assert(attr != NULL);

  return mr_attr_check_mapper_threads(attr->mapper_threads) &&
         mr_attr_check_reducer_threads(attr->reducer_threads) &&
         mr_attr_check_queue_size(attr->queue_size) &&
         mr_attr_check_log_file(attr->log_file);
}
