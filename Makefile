# Compiler and flags
CC := gcc
CFLAGS := -Wall -Wextra -std=c11 -g
LDFLAGS :=

# Directories
BUILD_DIR := build
SRC_DIR := src

# Source files and object files
SOURCES := $(SRC_DIR)/main.c $(SRC_DIR)/socket.c
OBJECTS := $(SOURCES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
EXECUTABLE := $(BUILD_DIR)/socket_discovery

# Default target
all: build

# Build target - compiles the executable
build: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "✅ Build complete: $(EXECUTABLE)"

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Run target - builds and runs the executable with optional arguments
run: build
	@echo "▶️ Running $(EXECUTABLE)..."
	./$(EXECUTABLE) $(ARGS)

# Clean target - removes all build artifacts
clean:
	@rm -rf $(BUILD_DIR)
	@echo "♻️ Clean complete: removed $(BUILD_DIR) directory"

# Phony targets
.PHONY: all build run clean
