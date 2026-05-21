# If using GNU Make do print the directory being processed
GNUMAKEFLAGS += --no-print-directory

CC = gcc
AR = ar
ARFLAGS = rcs
CFLAGS = $(warning_flags) $(addprefix -I, $(include_dirs))
BINARY = libmr.a

include_dirs = include
warning_flags = -Wall -Wextra -Wpedantic -Werror

src_dir = src
bin_dir = bin
int_dir = $(bin_dir)/intermediates

sources = $(wildcard $(src_dir)/*.c)
objects = $(sources:$(src_dir)/%.c=$(int_dir)/%.o)

.PHONY: all debug release build prepare clean

all: debug release

debug:
	@$(MAKE) build bin_dir=$(bin_dir)/debug \
		CFLAGS="$(CFLAGS) -g -Og -ggdb"

release:
	@$(MAKE) build bin_dir=$(bin_dir)/release \
		CFLAGS="$(CFLAGS) -O3"

build: prepare $(bin_dir)/$(BINARY)

$(bin_dir)/$(BINARY): $(objects)
	$(AR) $(ARFLAGS) $@ $^

$(int_dir)/%.o: $(src_dir)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

prepare:
	@mkdir -p $(bin_dir) $(int_dir)

clean:
	rm -rf $(bin_dir)/*
