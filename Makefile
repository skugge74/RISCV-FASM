# --- Toolchain ---
TARGET    := riscv-fasm
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

# --- Files & Formats ---
SRCS      := $(wildcard $(SRC_DIR)/*.c)
OBJS      := $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

# CLI Options
FILE      ?= tests/example.s
FORMAT    ?= flat
# Set QUIET=1 to silence the assembler UI (useful for automation)
QUIET     ?= 0
Q_FLAG    := $(if $(filter 1,$(QUIET)),-q,)

# Dynamically set objdump flags and expected output extension
ifeq ($(FORMAT),elf)
    EXT           := .o
    OBJDUMP_FLAGS := -d -M no-aliases
else
    EXT           := .bin
    OBJDUMP_FLAGS := -D -b binary -m riscv:rv32
endif

OUT_FILE := $(basename $(FILE))$(EXT)

# --- Rules ---

all: $(TARGET)

# Usage: make run FILE=tests/example.s [FORMAT=elf|flat] [QUIET=1]
run: $(TARGET)
	@if [ -f "$(FILE)" ]; then \
		echo "--- Assembling ($(FORMAT)) ---"; \
		./$(TARGET) $(Q_FLAG) -f $(FORMAT) -o $(OUT_FILE) $(FILE) || exit 1; \
		if [ "$(FORMAT)" = "elf" ]; then \
			echo "--- Linking ELF with GCC ---"; \
			riscv64-unknown-elf-gcc -march=rv32i -mabi=ilp32 -nostdlib $(OUT_FILE) -o $(OUT_FILE).elf || exit 1; \
			echo "--- Running QEMU ---"; \
			qemu-riscv32 ./$(OUT_FILE).elf; \
		else \
			echo "--- Running QEMU ---"; \
			$(QEMU) -M virt -bios none -kernel $(OUT_FILE) -nographic; \
		fi \
	else \
		echo "Error: File '$(FILE)' not found."; \
		exit 1; \
	fi

# Usage: make dump FILE=tests/example.s [FORMAT=elf|flat] [QUIET=1]
dump: $(TARGET)
	@if [ -f "$(FILE)" ]; then \
		./$(TARGET) $(Q_FLAG) -f $(FORMAT) -o $(OUT_FILE) $(FILE) && \
		echo "--- Disassembly ($(FORMAT)) ---" && \
		$(OBJDUMP) $(OBJDUMP_FLAGS) $(OUT_FILE); \
	else \
		echo "Error: File '$(FILE)' not found."; \
		exit 1; \
	fi

# Runs python test suite with Quiet Mode enabled for clean logs
test: clean all
	@QUIET=1 python3 test.py

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
	@rm -rf $(BUILD_DIR) $(TARGET) $(TEST_DIR)/*.bin $(TEST_DIR)/*.o

.PHONY: all run dump clean test
