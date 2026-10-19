# ==========================================
# 1. Directory & Variable Configuration
# ==========================================
CC     := gcc
CFLAGS := -Wall -Wextra -g -O3 -std=c23 -Iinclude -flto

# Directory Paths

# Where .c files go
SRC_DIR   := src

# Where intermediate .o files go
BUILD_DIR := build

# Where compiled binaries go
BIN_DIR   := bin

# Object files that every target needs
COMMON_OBJS := $(BUILD_DIR)/bitboard.o $(BUILD_DIR)/game_features.o

# Object files that anything using qlearning needs
QLEARNING_OBJS := $(COMMON_OBJS) $(BUILD_DIR)/qlearning.o $(BUILD_DIR)/file_handler.o

# Object files that anything using minimax needs
MINIMAX_OBJS := $(COMMON_OBJS) $(BUILD_DIR)/minimax.o


# All targets, these are what you can do, i.e. "make qlearning_train"
TARGETS := qlearning_train minimax_bench

# ==========================================
# 2. Build Rules
# ==========================================

# By default, does all targets, also if you do "make all"
all: $(TARGETS)

# Shared library for the Python graphics (ctypes)
bitboard_lib: $(COMMON_OBJS) | $(BIN_DIR)
	$(CC) $(CFLAGS) -shared -static-libgcc $^ -o $(BIN_DIR)/bitboard.dll

# When you do "make qlearning_train", compiles the object files after the colon if necessary
# The pipe at the end is necessary but don't worry about it
qlearning_train: $(BUILD_DIR)/qlearning_main.o $(QLEARNING_OBJS) | $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $(BIN_DIR)/$@

# Each of these has an entrypoint file which has a main, make sure to use correct $(BLAH_OBJS)
minimax_bench: $(BUILD_DIR)/minimax_main.o $(MINIMAX_OBJS) | $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $(BIN_DIR)/$@

bitboard_test: $(BUILD_DIR)/bitboard_main.o $(COMMON_OBJS) | $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $(BIN_DIR)/$@

# Compilation: Compiles each src/*.c into build/*.o
# The '| $(BUILD_DIR)' syntax ensures the build/ directory exists before running
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Directory Creation: Safely builds directories only if they don't exist
$(BUILD_DIR) $(BIN_DIR):
	mkdir -p $@

# ==========================================
# 3. Hygiene & Utilities
# ==========================================

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

.PHONY: all clean bitboard_lib
