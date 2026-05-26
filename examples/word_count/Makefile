CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -Werror -std=c11 -D_POSIX_C_SOURCE=200809L -I../../include

.PHONY: all clean

all: word_count

word_count: main.c ../../bin/debug/libmr.a
	$(CC) $(CFLAGS) main.c ../../bin/debug/libmr.a -o word_count

../../bin/debug/libmr.a:
	$(MAKE) -C ../.. debug

clean:
	rm -f word_count
