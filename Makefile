CC = gcc
AR = ar
ARFLAGS = rcs
CFLAGS = -Wall -Wextra -Wpedantic -Werror -Iinclude

SRC_DIR = src
BIN_DIR = bin
INT_DIR_NAME = intermediates
LIB_NAME = libmr.a

SRCS = $(wildcard $(SRC_DIR)/*.c)

.PHONY: all debug release clean

all: debug release

# Target-specific variables (inherited by prerequisites)
debug:   CFLAGS += -g -Og -ggdb
release: CFLAGS += -O3

debug:   $(BIN_DIR)/debug/$(LIB_NAME)
release: $(BIN_DIR)/release/$(LIB_NAME)

# Prerequisite definitions
$(BIN_DIR)/debug/$(LIB_NAME):   $(SRCS:$(SRC_DIR)/%.c=$(BIN_DIR)/debug/$(INT_DIR_NAME)/%.o)
$(BIN_DIR)/release/$(LIB_NAME): $(SRCS:$(SRC_DIR)/%.c=$(BIN_DIR)/release/$(INT_DIR_NAME)/%.o)

# Shared recipe for the libraries
$(BIN_DIR)/debug/$(LIB_NAME) $(BIN_DIR)/release/$(LIB_NAME):
	@mkdir -p $(dir $@)
	$(AR) $(ARFLAGS) $@ $^

# Rules for object files (specific to build type to handle directory structure)
$(BIN_DIR)/debug/$(INT_DIR_NAME)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN_DIR)/release/$(INT_DIR_NAME)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BIN_DIR)
