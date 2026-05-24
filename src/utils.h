#ifndef UTILS_H
#define UTILS_H

#define SYSCALL_CHECK(expression, command)                                     \
  do {                                                                         \
    if ((expression) == -1) {                                                  \
      command;                                                                 \
    }                                                                          \
  } while (0)

#define SYSCALL(result, expression, command)                                   \
  do {                                                                         \
    if ((result = (expression)) == -1) {                                       \
      command;                                                                 \
    }                                                                          \
  } while (0)

// For now it's just an alias
#define MRCALL_CHECK(expression, command) SYSCALL_CHECK(expression, command)

#endif // UTILS_H
