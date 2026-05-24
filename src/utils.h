#ifndef UTILS_H
#define UTILS_H

#define SYSCALL(result, expression, command)                                   \
  do {                                                                         \
    if ((result = (expression)) == -1) {                                       \
      command;                                                                 \
      return -1;                                                               \
    }                                                                          \
  } while (0)

#define SYSCALL_CHECK_CMD(expression, command)                                 \
  do {                                                                         \
    if ((expression) == -1) {                                                  \
      command;                                                                 \
      return -1;                                                               \
    }                                                                          \
  } while (0)

#define SYSCALL_CHECK(expression)                                              \
  do {                                                                         \
    if ((expression) == -1) {                                                  \
      return -1;                                                               \
    }                                                                          \
  } while (0)

// For now it's just aliases
#define MRCALL_CHECK_CMD(expression, command)                                  \
  SYSCALL_CHECK_CMD(expression, command)

#define MRCALL_CHECK(expression) SYSCALL_CHECK(expression)

#endif // UTILS_H
