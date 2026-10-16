# ==========================================
# 1. Directory & Variable Configuration
# ==========================================
CC     := clang
CFLAGS := -Wall -Wextra -Werror -g -O3

# Directory Paths
SRC_DIR   := src
BUILD_DIR := build
BIN_DIR   := bin

# Executable Output Path
TARGET := $(BIN_DIR)/program

# 1. Find all .c files inside the src/ folder (e.g., src/main.c)
SRCS := $(wildcard $(SRC_DIR)/*.c)

# 2. Transform the .c paths to .o paths inside the build/ folder
#    Example: src/main.c -> build/main.o
OBJS := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))

# ==========================================
# 2. Build Rules
# ==========================================

all: $(TARGET)

# Linking: Combines object files into the final binary inside bin/
# The '| $(BIN_DIR)' syntax ensures the bin/ directory exists before running
$(TARGET): $(OBJS) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

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

.PHONY: all clean
