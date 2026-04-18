# FLAT binary			 : make run FILE=Testing/FLATtests/while.s FORMAT=flat
# ELF C 					 : make run FILE=Testing/ELFtests/args_test.s FORMAT=elf
# RUN Python TESTS : make test 
#


# --- Toolchain & Directories ---
TARGET     := riscv-fasm
BUILD_DIR  := ./build
SRC_DIR    := ./src
INC_DIR    := ./include
TEST_DIR   := ./Testing

# Host Compiler
CC         := gcc
CFLAGS     := -I$(INC_DIR) -Wall -Wextra -Wpedantic -std=c11
DEBUG      := -g -O0

# RISC-V Cross-Tools
CROSS      := riscv64-unknown-elf-
GCC_RV     := $(CROSS)gcc
LD_RV      := $(CROSS)ld
OBJDUMP    := riscv64-linux-gnu-objdump
QEMU_SYS   := qemu-system-riscv32
QEMU_USR   := qemu-riscv32

# --- Files ---
SRCS       := $(wildcard $(SRC_DIR)/*.c)
OBJS       := $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

# CLI Defaults
FILE       ?= $(TEST_DIR)/FLATtests/example.s
FORMAT     ?= flat
QUIET      ?= 0
Q_FLAG     := $(if $(filter 1,$(QUIET)),-q,)

# Output Naming Logic
BASE       := $(basename $(FILE))
OUT_BIN    := $(BASE).bin
OUT_OBJ    := $(BASE).o
OUT_ELF    := $(BASE).elf

# --- Primary Rules ---

all: $(TARGET)

$(TARGET): $(OBJS)
	@echo "Linking $(TARGET)..."
	@$(CC) $(OBJS) -o $@ $(DEBUG)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	@echo "Compiling $<..."
	@$(CC) $(CFLAGS) $(DEBUG) -c $< -o $@

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

# --- Execution Rules ---

# Usage: make run FILE=Testing/FLATtests/while.s FORMAT=flat
# Usage: make run FILE=Testing/ELFtests/test.s FORMAT=elf
run: $(TARGET)
	@if [ ! -f "$(FILE)" ]; then echo "Error: $(FILE) not found"; exit 1; fi
	
	@if [ "$(FORMAT)" = "elf" ]; then \
		echo "[1/3] Assembling ELF: $(FILE)"; \
		./$(TARGET) $(Q_FLAG) -f elf -o $(OUT_OBJ) $(FILE) || exit 1; \
		echo "[2/3] Linking with GCC"; \
		$(GCC_RV) -march=rv32i -mabi=ilp32 -nostdlib $(OUT_OBJ) -o $(OUT_ELF) || exit 1; \
		echo "[3/3] Running QEMU User Mode"; \
		$(QEMU_USR) ./$(OUT_ELF); \
	else \
		echo "[1/2] Assembling Flat: $(FILE)"; \
		./$(TARGET) $(Q_FLAG) -f flat -o $(OUT_BIN) $(FILE) || exit 1; \
		echo "[2/2] Running QEMU System Mode"; \
		$(QEMU_SYS) -M virt -bios none -kernel $(OUT_BIN) -nographic; \
	fi

# Usage: make dump FILE=Testing/FLATtests/math.s FORMAT=flat
dump: $(TARGET)
	@if [ "$(FORMAT)" = "elf" ]; then \
		./$(TARGET) $(Q_FLAG) -f elf -o $(OUT_OBJ) $(FILE) && \
		$(OBJDUMP) -d -M no-aliases $(OUT_OBJ); \
	else \
		./$(TARGET) $(Q_FLAG) -f flat -o $(OUT_BIN) $(FILE) && \
		$(OBJDUMP) -D -b binary -m riscv:rv32 $(OUT_BIN); \
	fi

# --- Maintenance ---

test: all
	@cd Testing && python3 test.py

clean:
	@echo "Cleaning build artifacts..."
	@rm -rf $(BUILD_DIR) $(TARGET)
	@find $(TEST_DIR) -type f \( -name "*.bin" -name "*.o" -name "*.elf" -name "*.elf_exe" \) -delete

.PHONY: all run dump clean test
