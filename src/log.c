#include "log.h"

#include <assert.h>
#include <fcntl.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "utils.h"

int mr_log_init(mr_log_file_t* log, const char* log_file_path) {
  assert(log != NULL);
  assert(log_file_path != NULL);

  if ((log->file = fopen(log_file_path, "a")) == NULL) {
    // errno is set by fopen
    return -1;
  }

  if ((log->sem = sem_open(MR_LOG_SEM_NAME, O_CREAT | O_RDWR, 0600,
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

  SYSCALL_CHECK(sem_wait(log->sem));

  time_t now = time(NULL);

  // time returns -1 on error we can use SYSCALL_CHECK_CMD
  SYSCALL_CHECK_CMD(now, sem_post(log->sem));

  int result;

#if MR_LOG_ENABLE_TIMESTAMP
  char time_buf[64];
  if (!strftime(time_buf, sizeof(time_buf), "%c", localtime(&now)))
    time_buf[sizeof(time_buf) - 1] =
        '\0'; // Fallback to truncated time if formatting fails

  result = fprintf(log->file, "%s %s[%d] %s %s: ", time_buf, pname, getpid(),
                   tname, level);
#else
  result = fprintf(log->file, "%s[%d] %s %s: ", pname, getpid(), tname, level);
#endif

  if (result < 0) {
    // errno is set by fprintf;
    sem_post(log->sem);
    return -1;
  }

  // format the message
  va_list args;
  va_start(args, format);
  result = vfprintf(log->file, format, args);
  va_end(args);

  if (result < 0) {
    // errno is set by vfprintf
    sem_post(log->sem);
    return -1;
  }

  if (fputc('\n', log->file) == EOF) {
    // errno is set by fputc
    sem_post(log->sem);
    return -1;
  }

  if (fflush(log->file) < 0) { // Ensure logs are written to disk
    // errno is set by fflush
    sem_post(log->sem);
  }

  SYSCALL_CHECK(sem_post(log->sem));

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

  SYSCALL_CHECK_CMD(sem_close(log->sem), {
    sem_unlink(MR_LOG_SEM_NAME);
    log->file = NULL;
    log->sem = SEM_FAILED;
  });

  SYSCALL_CHECK_CMD(sem_unlink(MR_LOG_SEM_NAME), {
    log->file = NULL;
    log->sem = SEM_FAILED;
  });

  log->file = NULL;
  log->sem = SEM_FAILED;

  return 0;
}
