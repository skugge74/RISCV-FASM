# --- Toolchain ---
TARGET    := riscv-assembler
BUILD_DIR := ./build
SRC_DIR   := ./src
INC_DIR   := ./include
TEST_DIR  := ./tests

CC        := gcc
CFLAGS    := -I$(INC_DIR) -Wall -Wextra -Wpedantic -std=c11
DEBUG     := -g -O0

# RISC-V Tools
OBJDUMP   := riscv64-linux-gnu-objdump
QEMU      := qemu-system-riscv32

# --- Files ---
SRCS      := $(wildcard $(SRC_DIR)/*.c)
OBJS      := $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

# Default assembly file if none provided
FILE    ?= tests/example.s
HEX_OUT := $(FILE:.s=.bin)

# --- Rules ---

all: $(TARGET)

# 1. Assemble and Run in QEMU
# Usage: make run FILE=macros/data.s
run: $(TARGET)
	@if [ -f "$(FILE)" ]; then \
		echo "--- Assembling ---"; \
		./$(TARGET) $(FILE) $(HEX_OUT); \
		echo "--- Running QEMU ---"; \
		$(QEMU) -M virt -bios none -kernel $(HEX_OUT) -nographic; \
	else \
		echo "Error: File '$(FILE)' not found."; \
		exit 1; \
	fi

# 2. Assemble and Dump Disassembly
# Usage: make dump FILE=macros/data.s
dump: $(TARGET)
	@if [ -f "$(FILE)" ]; then \
		./$(TARGET) $(FILE) $(HEX_OUT); \
		echo "--- Disassembly ---"; \
		$(OBJDUMP) -D -b binary -m riscv:rv32 $(HEX_OUT); \
	else \
		echo "Error: File '$(FILE)' not found."; \
		exit 1; \
	fi

# 3. Build the Assembler itself
$(TARGET): $(OBJS)
	@echo "Linking $(TARGET)..."
	@$(CC) $(OBJS) -o $@ $(DEBUG)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	@echo "Compiling $<..."
	@$(CC) $(CFLAGS) $(DEBUG) -c $< -o $@

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

clean:
	@echo "Cleaning build artifacts..."
	@rm -rf $(BUILD_DIR) $(TARGET) $(TEST_DIR)/*.bin

.PHONY: all run dump clean
