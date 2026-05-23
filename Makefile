# If using GNU Make do print the directory being processed
GNUMAKEFLAGS += --no-print-directory

CC = gcc
AR = ar
ARFLAGS = rcs
CFLAGS = $(warning_flags) $(addprefix -I, $(include_dirs)) -std=c11
BINARY = libmr.a

include_dirs = include
warning_flags = -Wall -Wextra -Wpedantic -Werror

debug_flags = -g -Og -ggdb -DMR_DEBUG
release_flags = -O3 -DNDEBUG

src_dir = src
bin_dir = bin
lib_dir = lib
int_dir = $(bin_dir)/intermediates

.PHONY: all debug release build prepare clean test test-debug test-release run-tests

all: debug release

debug:
	@echo "Building in debug mode..."
	@$(MAKE) build bin_dir=$(bin_dir)/debug \
		CFLAGS="$(CFLAGS) $(debug_flags)"

release:
	@echo "Building in release mode..."
	@$(MAKE) build bin_dir=$(bin_dir)/release \
		CFLAGS="$(CFLAGS) $(release_flags)"

test: test-debug test-release

test-debug:
	@echo "Testing in debug mode..."
	@$(MAKE) run-tests bin_dir=$(bin_dir)/debug \
		CFLAGS="$(CFLAGS) $(debug_flags)"

test-release:
	@echo "Testing in release mode..."
	@$(MAKE) run-tests bin_dir=$(bin_dir)/release \
		CFLAGS="$(CFLAGS) $(release_flags)"

clean:
	rm -rf $(bin_dir)/*

# -------------- Library Builder ------------------
src_int_dir = $(int_dir)/src

sources = $(wildcard $(src_dir)/*.c)
objects = $(sources:$(src_dir)/%.c=$(src_int_dir)/%.o)

build: prepare $(bin_dir)/$(BINARY)

$(bin_dir)/$(BINARY): $(objects)
	$(AR) $(ARFLAGS) $@ $^

$(src_int_dir)/%.o: $(src_dir)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# --------------- Test Runner ---------------------
test_include_dirs = $(unity_dir)/src

TEST_CFLAGS = $(addprefix -I, $(test_include_dirs))

test_dir = test
test_int_dir = $(int_dir)/test
unity_dir = $(lib_dir)/vendor/Unity

test_sources = $(wildcard $(test_dir)/*.c)
test_objects = $(test_sources:$(test_dir)/%.c=$(test_int_dir)/%.o)

run-tests: prepare $(bin_dir)/test_runner
	@./$(bin_dir)/test_runner

$(bin_dir)/test_runner: $(test_objects) $(test_int_dir)/unity.o $(bin_dir)/$(BINARY)
	@$(CC) $(CFLAGS) $(TEST_CFLAGS) $^ -o $@

$(test_int_dir)/unity.o: $(unity_dir)/src/unity.c
	@$(CC) $(CFLAGS) -c $< -o $@

$(test_int_dir)/%.o: $(test_dir)/%.c
	@$(CC) $(CFLAGS) $(TEST_CFLAGS) -c $< -o $@

# -------------- Utility Targets ------------------
prepare:
	@mkdir -p $(bin_dir) $(src_int_dir) $(test_int_dir)
