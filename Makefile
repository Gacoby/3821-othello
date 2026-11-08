# ==========================================
# 1. Directory & Variable Configuration
# ==========================================
CC     := gcc

# ------------------------------------------
# Platform detection
# ------------------------------------------
# Must match the names cboard.py looks for in bin/
ifeq ($(OS),Windows_NT)
    PLATFORM  := windows
    ARCH      := x86_64
    LIB_NAME  := bitboard.dll
    SHARED_FLAGS := -shared -static-libgcc
    PIC_FLAGS :=
else
    UNAME_S := $(shell uname -s)
    ARCH    := $(shell uname -m)
    PIC_FLAGS := -fPIC
    ifeq ($(UNAME_S),Darwin)
        PLATFORM  := mac
        LIB_NAME  := libbitboard.dylib
        SHARED_FLAGS := -dynamiclib
    else
        PLATFORM  := linux
        LIB_NAME  := libbitboard.so
        SHARED_FLAGS := -shared
    endif
endif

# -mpopcnt is x86-only (it is an error on Apple Silicon / ARM).
# On ARM the compiler emits the equivalent instructions by default.
ifneq (,$(filter x86_64 amd64 AMD64 i386 i686,$(ARCH)))
    ARCH_FLAGS := -mpopcnt
else
    ARCH_FLAGS :=
endif

# -MMD -MP generate .d dependency files so that editing a header (e.g. minimax.h)
# rebuilds every .o that includes it
CFLAGS := -Wall -Wextra -g -O3 -std=c23 -Iinclude -flto -MMD -MP $(ARCH_FLAGS) $(PIC_FLAGS)
LDLIBS = -lm

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
QLEARNING_OBJS := $(COMMON_OBJS) $(BUILD_DIR)/qlearning.o

# Object files that anything using minimax needs
# (minimax.c needs the bitboard API and the transposition table)
MINIMAX_OBJS := $(COMMON_OBJS) $(BUILD_DIR)/transposition_table.o $(BUILD_DIR)/minimax.o


# All targets, these are what you can do, i.e. "make qlearning_train"
TARGETS := qlearning_train minimax_bench

# ==========================================
# 2. Build Rules
# ==========================================

# Plain "make" only builds the shared library for now, while qlearning is unfinished
# Switch this back to "all" once everything compiles
.DEFAULT_GOAL := bitboard_lib

# Does all targets, i.e. "make all"
all: $(TARGETS)

# Shared library for the Python graphics (ctypes)
bitboard_lib: $(MINIMAX_OBJS) | $(BIN_DIR)
	$(CC) $(CFLAGS) $(SHARED_FLAGS) $^ -o $(BIN_DIR)/$(LIB_NAME)

# When you do "make qlearning_train", compiles the object files after the colon if necessary
# The pipe at the end is necessary but don't worry about it
qlearning_train: $(BUILD_DIR)/qlearning_main.o $(QLEARNING_OBJS) | $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $(BIN_DIR)/$@ $(LDLIBS)

# Each of these has an entrypoint file which has a main, make sure to use correct $(BLAH_OBJS)
# src/minimax_main.c should call timedMinimax() from include/minimax.h
minimax_bench: $(BUILD_DIR)/minimax_main.o $(MINIMAX_OBJS) | $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $(BIN_DIR)/$@

bitboard_test: $(BUILD_DIR)/bitboard_main.o $(COMMON_OBJS) | $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $(BIN_DIR)/$@

validator: $(BUILD_DIR)/validator.o $(QLEARNING_OBJS) $(MINIMAX_OBJS) | $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $(BIN_DIR)/$@

tournament: $(BUILD_DIR)/tournament.o $(COMMON_OBJS) | $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $(BIN_DIR)/$@

# Compilation: Compiles each src/*.c into build/*.o
# The '| $(BUILD_DIR)' syntax ensures the build/ directory exists before running
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Directory Creation: Safely builds directories only if they don't exist
$(BUILD_DIR) $(BIN_DIR):
	mkdir -p $@

# Pull in the generated header dependencies (ignored if they don't exist yet)
-include $(wildcard $(BUILD_DIR)/*.d)

# ==========================================
# 3. Hygiene & Utilities
# ==========================================

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

# These names are commands, not files they produce
.PHONY: all clean bitboard_lib $(TARGETS) bitboard_test