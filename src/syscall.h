#ifndef SYSCALL_H
#define SYSCALL_H

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

#endif // SYSCALL_H
