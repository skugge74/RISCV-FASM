TARGET    := riscv-assembler
BUILD_DIR := ./build
SRC_DIR   := ./src
INC_DIR   := ./include
TEST_DIR  := ./macros
CC        := gcc
CFLAGS    := -I$(INC_DIR) -Wall -Wextra -Wpedantic -std=c11
DEBUG     := -g -O0
RELEASE   := -O2

SRCS      := $(wildcard $(SRC_DIR)/*.c)
OBJS      := $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

FILE ?= macros/repeat.s
HEX_OUT := $(FILE:.s=.hex)

all: $(TARGET)

run: $(TARGET)
	@if [ -f "$(FILE)" ]; then \
		echo "Assembling $(FILE) -> $(HEX_OUT)..."; \
		./$(TARGET) $(FILE) $(HEX_OUT); \
		echo "Output saved to $(HEX_OUT)"; \
	else \
		echo "Error: File '$(FILE)' not found."; \
		echo "Usage: make run FILE=path/to/file.s"; \
		exit 1; \
	fi


$(TARGET): $(OBJS)
	@$(CC) $(OBJS) -o $@ $(DEBUG)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	@$(CC) $(CFLAGS) $(DEBUG) -c $< -o $@

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

clean:
	@rm -rf $(BUILD_DIR) $(TARGET) $(TEST_DIR)/*.hex

.PHONY: all run clean
