#include "log.h"

#include <assert.h>
#include <fcntl.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "syscall.h"

int mr_log_init(mr_log_file_t* log, const char* log_file_path) {
  assert(log != NULL);
  assert(log_file_path != NULL);

  if ((log->file = fopen(log_file_path, "a")) == NULL) {
    // errno is set by fopen
    return -1;
  }

  if ((log->sem = sem_open(MR_LOG_SEM_NAME, O_CREAT | O_EXCL | O_RDWR, 0600,
                            1)) == SEM_FAILED) {
    // errno is set by sem_open
    fclose(log->file);
    log->file = NULL;
    return -1;
  }

  return 0;
}

int mr_log(mr_log_file_t* log, const char* level, const char* pname,
           const char* tname, const char* format, ...) {
  assert(log != NULL);
  assert(log->file != NULL);
  assert(log->sem != NULL);
  assert(format != NULL);

  SYSCALL_CHECK(sem_wait(log->sem), return -1);

  time_t now = time(NULL);

  if (now == (time_t)-1) {
    // errno is set by time
    sem_post(log->sem);
    return -1;
  }

  if (fprintf(log->file, "%s %s[%d] %s %s: ", asctime(localtime(&now)), pname,
              getpid(), tname, level) < 0) {
    // errno is set by fprintf;
    sem_post(log->sem);
    return -1;
  }

  // format the message
  va_list args;
  va_start(args, format);
  int result = vfprintf(log->file, format, args);
  va_end(args);

  if (result < 0) {
    // errno is set by vfprintf
    sem_post(log->sem);
    return -1;
  }

  if (fflush(log->file) < 0) { // Ensure logs are written to disk
    // errno is set by fflush
    sem_post(log->sem);
  }

  SYSCALL_CHECK(sem_post(log->sem), return -1);

  return 0;
}

int mr_log_destroy(mr_log_file_t* log) {
  assert(log != NULL);
  assert(log->file != NULL);
  assert(log->sem != NULL);

  if (fclose(log->file) == EOF) {
    // errno is set by fclose
    sem_close(log->sem);
    sem_unlink(MR_LOG_SEM_NAME);
    log->file = NULL;
    log->sem = SEM_FAILED;
    return -1;
  }

  SYSCALL_CHECK(sem_close(log->sem), {
    sem_unlink(MR_LOG_SEM_NAME);
    log->file = NULL;
    log->sem = SEM_FAILED;
    return -1;
  });

  SYSCALL_CHECK(sem_unlink(MR_LOG_SEM_NAME), {
    log->file = NULL;
    log->sem = SEM_FAILED;
    return -1;
  });

  log->file = NULL;
  log->sem = SEM_FAILED;

  return 0;
}
