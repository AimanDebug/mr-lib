#ifndef UTILS_H
#define UTILS_H

#define SYSCALL_RET(result, expression, command)                               \
  do {                                                                         \
    if ((result = (expression)) == -1) {                                       \
      command;                                                                 \
      return -1;                                                               \
    }                                                                          \
  } while (0)

#define SYSCALL_RET_CHECK(expression, command)                                 \
  do {                                                                         \
    if ((expression) == -1) {                                                  \
      command;                                                                 \
      return -1;                                                               \
    }                                                                          \
  } while (0)

#define SYSCALL_EXIT(result, expression, command)                              \
  do {                                                                         \
    if ((result = (expression)) == -1) {                                       \
      command;                                                                 \
      return EXIT_FAILURE;                                                     \
    }                                                                          \
  } while (0)

#define SYSCALL_EXIT_CHECK(expression, command)                                \
  do {                                                                         \
    if ((expression) == -1) {                                                  \
      command;                                                                 \
      return EXIT_FAILURE;                                                     \
    }                                                                          \
  } while (0)

#endif // UTILS_H
