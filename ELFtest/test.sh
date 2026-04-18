#!/bin/bash

# --- Configuration ---
ASSEMBLER="../riscv-fasm"
ASM_FILE="final_asm.s"
OBJ_FILE="final_asm.o"
C_FILE="main.c"
EXEC_FILE="final_test"
EXPECTED_EXIT=100

# --- Colors ---
RED='\033[1;31m'
GREEN='\033[1;32m'
YELLOW='\033[1;33m'
CYAN='\033[1;36m'
NC='\033[0m' # No Color

echo -e "${CYAN}================================================${NC}"
echo -e "${CYAN}  Running Kdex 'Ultimate Boss' C-Interop Test   ${NC}"
echo -e "${CYAN}================================================${NC}"

# 1. Check Dependencies
if ! command -v riscv64-unknown-elf-gcc &> /dev/null; then
    echo -e "${RED}✘ Error: riscv64-unknown-elf-gcc is not installed or not in PATH.${NC}"
    exit 1
fi
if ! command -v qemu-riscv32 &> /dev/null; then
    echo -e "${RED}✘ Error: qemu-riscv32 is not installed or not in PATH.${NC}"
    exit 1
fi

# 2. Assemble the RISC-V file using your custom assembler
echo -e "\n${YELLOW}[1/3] Assembling ${ASM_FILE} with Kdex...${NC}"
$ASSEMBLER $ASM_FILE -f elf -o $OBJ_FILE

if [ $? -ne 0 ] || [ ! -f "$OBJ_FILE" ]; then
    echo -e "${RED}✘ BUILD FAILED: Custom Assembler returned an error.${NC}"
    exit 1
fi

# Print Relocations to prove the math addend is there
echo -e "${CYAN}--- ELF Relocation Table ---${NC}"
readelf -r $OBJ_FILE | grep -A 3 "Relocation section"
echo -e "${CYAN}----------------------------${NC}"

# 3. Compile the C file and link it using GCC
echo -e "\n${YELLOW}[2/3] Linking ${C_FILE} and ${OBJ_FILE} with GCC...${NC}"
riscv64-unknown-elf-gcc -march=rv32i -mabi=ilp32 -nostdlib $C_FILE $OBJ_FILE -o $EXEC_FILE

if [ $? -ne 0 ] || [ ! -f "$EXEC_FILE" ]; then
    echo -e "${RED}✘ LINK FAILED: GCC Linker returned an error.${NC}"
    exit 1
fi

# 4. Run the hybrid program in QEMU and capture the exit code
echo -e "${YELLOW}[3/3] Executing bare-metal binary in QEMU...${NC}"
qemu-riscv32 ./$EXEC_FILE
RESULT=$?

# 5. Verify the Result
echo -e "\n──────────────────────────────────────────"
if [ $RESULT -eq $EXPECTED_EXIT ]; then
    echo -e "${GREEN}✔ ULTIMATE TEST PASSED!${NC}"
    echo -e "  Expected Exit Code: ${EXPECTED_EXIT}"
    echo -e "  Actual Exit Code:   ${GREEN}${RESULT}${NC}"
    
    # Cleanup on success
    rm -f $OBJ_FILE $EXEC_FILE
    exit 0
else
    echo -e "${RED}✘ ULTIMATE TEST FAILED!${NC}"
    echo -e "  Expected Exit Code: ${EXPECTED_EXIT}"
    echo -e "  Actual Exit Code:   ${RED}${RESULT}${NC}"
    
    # Leave artifacts for debugging
    exit 1
fi
