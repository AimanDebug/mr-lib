# If using GNU Make do print the directory being processed
GNUMAKEFLAGS += --no-print-directory

CC = gcc
AR = ar
ARFLAGS = rcs
CFLAGS = $(warning_flags) $(addprefix -I, $(include_dirs)) -std=c11 -D_POSIX_C_SOURCE=200809L
TEST_CFLAGS = $(addprefix -I, $(test_include_dirs)) -DMR_LOG_ENABLE_TIMESTAMP=0
BINARY = libmr.a

include_dirs = include src
test_include_dirs = $(unity_dir)/src
warning_flags = -Wall -Wextra -Wpedantic -Werror

debug_flags = -g -Og -ggdb -DMR_DEBUG
release_flags = -O3 -DNDEBUG

src_dir = src
bin_dir = bin
lib_dir = lib
int_dir = $(bin_dir)/intermediates

.PHONY: all debug release build prepare clean test test-debug test-release run-tests docs

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
		CFLAGS="$(CFLAGS) $(debug_flags) $(TEST_CFLAGS)"

test-release:
	@echo "Testing in release mode..."
	@$(MAKE) run-tests bin_dir=$(bin_dir)/release \
		CFLAGS="$(CFLAGS) $(release_flags) $(TEST_CFLAGS)"

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
test_dir = test
test_int_dir = $(int_dir)/test
unity_dir = $(lib_dir)/vendor/Unity

test_subdirs = $(wildcard $(test_dir)/*/)
test_runners = $(patsubst $(test_dir)/%/, $(bin_dir)/test_%, $(test_subdirs))

run-tests: prepare $(test_runners)
	@for runner in $(test_runners); do \
		echo "Running $$runner..."; \
		./$$runner || exit 1; \
	done

$(bin_dir)/test_%: $(test_int_dir)/%/main.o $(test_int_dir)/unity.o $(bin_dir)/$(BINARY)
	@$(CC) $(CFLAGS) $^ -o $@

$(test_int_dir)/unity.o: $(unity_dir)/src/unity.c
	@$(CC) $(CFLAGS) -c $< -o $@

$(test_int_dir)/%.o: $(test_dir)/%.c
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -c $< -o $@

# -------------- Utility Targets ------------------
prepare:
	@mkdir -p $(bin_dir) $(src_int_dir) $(test_int_dir)

# -------------- Documentation --------------------
DOC_DIR = docs
DOC_NAME = Relazione

docs: $(DOC_DIR)/$(DOC_NAME).tex
	@echo "Compiling documentation..."
	@cd $(DOC_DIR) && pdflatex -interaction=nonstopmode $(DOC_NAME).tex > /dev/null
	@cd $(DOC_DIR) && pdflatex -interaction=nonstopmode $(DOC_NAME).tex > /dev/null
	@rm -f $(DOC_DIR)/$(DOC_NAME).aux $(DOC_DIR)/$(DOC_NAME).log $(DOC_DIR)/$(DOC_NAME).out
	@echo "Documentation compiled: $(DOC_DIR)/$(DOC_NAME).pdf"
