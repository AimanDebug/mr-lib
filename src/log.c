#include "log.h"

#include <assert.h>
#include <fcntl.h>
#include <semaphore.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "syscall.h"
#include <config.h>

FILE* g_log_file = NULL;
sem_t* g_log_sem = NULL;

int mr_log_init(const char* log_file_path) {
  assert(log_file_path != NULL);

  if ((g_log_file = fopen(log_file_path, "a")) == NULL) {
    // errno is set by fopen
    return -1;
  }

  if ((g_log_sem = sem_open(MR_LOG_SEM_NAME, O_CREAT | O_EXCL | O_RDWR, 0600,
                            1)) == SEM_FAILED) {
    // errno is set by sem_open
    fclose(g_log_file);
    return -1;
  }

  return 0;
}

int mr_log(const char* level, const char* pname, const char* tname,
           const char* format, ...) {
  assert(g_log_file != NULL);
  assert(g_log_sem != NULL);
  assert(format != NULL);

  SYSCALL_CHECK(sem_wait(g_log_sem), return -1);

  time_t now = time(NULL);

  if (now == (time_t)-1) {
    // errno is set by time
    sem_post(g_log_sem);
    return -1;
  }

  if (fprintf(g_log_file, "%s %s[%d] %s %s: ",
              asctime(localtime(&now)), pname, getpid(), tname, level) < 0) {
    // errno is set by fprintf;
    sem_post(g_log_sem);
    return -1;
  }

  // format the message
  va_list args;
  va_start(args, format);
  int result = vfprintf(g_log_file, format, args);
  va_end(args);

  if (result < 0) {
    // errno is set by vfprintf
    sem_post(g_log_sem);
    return -1;
  }

  if (fflush(g_log_file) < 0) { // Ensure logs are written to disk
    // errno is set by fflush
    sem_post(g_log_sem);
  }

  SYSCALL_CHECK(sem_post(g_log_sem), return -1);

  return 0;
}

int mr_log_destroy(void) {
  assert(g_log_file != NULL);
  assert(g_log_sem != NULL);

  if (fclose(g_log_file) == EOF) {
    // errno is set by fclose
    sem_close(g_log_sem);
    sem_unlink(MR_LOG_SEM_NAME);
    return -1;
  }

  SYSCALL_CHECK(sem_close(g_log_sem), {
    sem_unlink(MR_LOG_SEM_NAME);
    return -1;
  });

  SYSCALL_CHECK(sem_unlink(MR_LOG_SEM_NAME), return -1);

  return 0;
}
