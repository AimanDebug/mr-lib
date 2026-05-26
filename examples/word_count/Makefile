CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -Werror -std=c11 -D_POSIX_C_SOURCE=200809L -I../../include

.PHONY: all clean

all: word_count

word_count: main.c ../../bin/release/libmr.a
	$(CC) $(CFLAGS) main.c ../../bin/release/libmr.a -o word_count

../../bin/release/libmr.a:
	$(MAKE) -C ../.. release

clean:
	rm -f word_count
