#!/bin/bash

# --- Configuration ---
ASSEMBLER="../riscv-fasm"  # Change this if your binary is named differently
TEST_FILE="./test.s"
OBJ_FILE="./test.o"

# --- Colors ---
RED='\033[1;31m'
GREEN='\033[1;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo -e "${YELLOW}--- Running ELF Format Integration Test ---${NC}"

# 1. Ensure the assembler exists
if [ ! -f "$ASSEMBLER" ]; then
    echo -e "${RED}✘ Error: Assembler binary '$ASSEMBLER' not found.${NC}"
    exit 1
fi

# 2. Run the assembler
echo "Assembling $TEST_FILE to ELF..."
$ASSEMBLER $TEST_FILE -f elf -o $OBJ_FILE

if [ $? -ne 0 ]; then
    echo -e "${RED}✘ BUILD FAILED: Assembler returned an error.${NC}"
    exit 1
fi

if [ ! -f "$OBJ_FILE" ]; then
    echo -e "${RED}✘ BUILD FAILED: $OBJ_FILE was not created.${NC}"
    exit 1
fi

# --- 3. Verify Relocations ---
echo "Checking Relocation Table..."
RELOCS=$(readelf -r $OBJ_FILE 2>/dev/null)

# Count total RISC-V relocations (Should be exactly 5)
RELOC_COUNT=$(echo "$RELOCS" | grep -c "R_RISCV")
if [ "$RELOC_COUNT" -ne 5 ]; then
    echo -e "${RED}✘ FAIL: Expected 5 relocations, found $RELOC_COUNT.${NC}"
    exit 1
fi

# Check for specific relocations
if ! echo "$RELOCS" | grep -q "R_RISCV_HI20.*external_variable"; then
    echo -e "${RED}✘ FAIL: Missing HI20 relocation for 'external_variable'${NC}"
    exit 1
fi
if ! echo "$RELOCS" | grep -q "R_RISCV_LO12_I.*external_variable"; then
    echo -e "${RED}✘ FAIL: Missing LO12_I relocation for 'external_variable'${NC}"
    exit 1
fi
if ! echo "$RELOCS" | grep -q "R_RISCV_JAL.*exit_program"; then
    echo -e "${RED}✘ FAIL: Missing JAL relocation for 'exit_program'${NC}"
    exit 1
fi

# --- 4. Verify Symbol Table ---
echo "Checking Symbol Table..."
SYMBOLS=$(readelf -s $OBJ_FILE 2>/dev/null)

# Assert external symbols are marked as UND (Undefined)
for SYM in "external_variable" "printf" "exit_program"; do
    if ! echo "$SYMBOLS" | grep -q "UND $SYM"; then
        echo -e "${RED}✘ FAIL: Symbol '$SYM' is not marked as UND (Undefined).${NC}"
        exit 1
    fi
done

# Assert _start is defined (Does NOT have UND)
if echo "$SYMBOLS" | grep " _start" | grep -q "UND"; then
    echo -e "${RED}✘ FAIL: Symbol '_start' should be defined, but is marked UND.${NC}"
    exit 1
fi

# Assert local labels are NOT in the table as undefined globals
if echo "$SYMBOLS" | grep -q "UND.*local_loop"; then
    echo -e "${RED}✘ FAIL: Local label '.local_loop' leaked into the symbol table as UND.${NC}"
    exit 1
fi

# --- Success ---
echo -e "${GREEN}✔ ELF Integration Test Passed!${NC}"
echo "  - 5/5 Relocations verified."
echo "  - External symbols correctly marked UND."
echo "  - Local symbols resolved internally."

# Cleanup
rm -f $OBJ_FILE
exit 0
