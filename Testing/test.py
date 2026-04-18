#!/usr/bin/env python3
import os
import subprocess
import re
import time
import sys

# --- Configuration ---
# Absolute path to the built assembler binary
ASSEMBLER = os.path.abspath("../riscv-fasm") 
FLAT_DIR = "FLATtests"
ELF_DIR = "ELFtests"
MODULAR_DIR = "ModularLinkingTests"
SKIP_FILES = ["included.s", "big_macro.s", "macros.s"] 

# Get absolute path of the Testing directory
TESTING_ROOT = os.path.dirname(os.path.abspath(__file__))

PASSED_COUNT = 0
FAILED_COUNT = 0

# --- Helper Functions ---

def strip_ansi(text):
    """Removes ANSI escape codes from strings."""
    ansi_escape = re.compile(r'\x1B(?:[@-Z\\-_]|\[[0-?]*[ -/]*[@-~])')
    return ansi_escape.sub('', text)

def normalize_output(text):
    """Cleans up the output for comparison by removing UI boxes and headers."""
    text = strip_ansi(text)
    # Remove UI Box drawing characters
    text = re.sub(r'[┌┐└┘│─┤├┬┴]', '', text)
    # Remove the UI header box text specifically
    text = re.sub(r'RISC-V-FASM - Toolchain Architecture.*?\n', '', text, flags=re.DOTALL)
    # Remove specific UI keywords
    text = re.sub(r'(Pre-processing:|Encoding Instructions:|Writing ELF to|Build Successful|Output:|Format:|Size:|Time:).*?\n', '', text)
    # Generic placeholder for line numbers to prevent pass/fail mismatch on line drift
    text = re.sub(r':\d+\b', ':XX', text)
    lines = text.splitlines()
    return "\n".join([l.strip() for l in lines if l.strip()]).strip()

def print_result(file, passed, details=""):
    global PASSED_COUNT, FAILED_COUNT
    if passed:
        PASSED_COUNT += 1
        print(f" \033[92m✔ {file} [PASSED]\033[0m {details}")
    else:
        FAILED_COUNT += 1
        print(f" \033[91m✘ {file} [FAILED]\033[0m")
        if details: 
            print(f"----------------------------------------\n{details}\n----------------------------------------")

# --- Test Execution Logic ---

def run_flat_test(filename):
    """Assembles and runs flat binaries directly using QEMU-System."""
    cwd = os.path.join(TESTING_ROOT, FLAT_DIR)
    bin_file = filename.replace('.s', '.bin')
    test_file = filename.replace('.s', '.test')
    input_file = filename.replace('.s', '.in')

    if not os.path.exists(os.path.join(cwd, test_file)): return

    try:
        # 1. Assemble
        # We run this directly to avoid Makefile pathing issues
        res = subprocess.run([ASSEMBLER, "-f", "flat", "-o", bin_file, filename], 
                             cwd=cwd, capture_output=True, text=True)
        
        with open(os.path.join(cwd, test_file), "r") as f:
            expected = normalize_output(f.read())
        
        # Check for expected assembly errors
        if "[ERROR]" in expected:
            actual = normalize_output(res.stdout + res.stderr)
            if expected in actual: print_result(filename, True)
            else: print_result(filename, False, f"Expected Error:\n{expected}\n\nActual:\n{actual}")
            return

        if res.returncode != 0:
            print_result(filename, False, f"Assembler Failed:\n{res.stderr}")
            return

        # 2. Handle Input Data (.in files)
        input_data = None
        if os.path.exists(os.path.join(cwd, input_file)):
            with open(os.path.join(cwd, input_file), "r") as f:
                input_data = f.read()

        # 3. Run QEMU (System Mode)
        # Using -display none and -serial stdio is safer than -nographic in some Python environments
        qemu_cmd = ["qemu-system-riscv32", "-M", "virt", "-bios", "none", "-kernel", bin_file, "-nographic"]
        
        proc = subprocess.run(qemu_cmd, cwd=cwd, input=input_data, capture_output=True, text=True, timeout=5)
        
        actual_out = normalize_output(proc.stdout)
        if actual_out == expected:
            print_result(filename, True)
        else:
            print_result(filename, False, f"Expected:\n{expected}\n\nActual:\n{actual_out}")

        # Cleanup
        if os.path.exists(os.path.join(cwd, bin_file)): os.remove(os.path.join(cwd, bin_file))

    except subprocess.TimeoutExpired:
        print_result(filename, False, "TIMEOUT: QEMU ran for too long (Possible infinite loop)")
    except Exception as e:
        print_result(filename, False, f"Error: {e}")
    finally:
        # This restores terminal echo and cursor if QEMU crashes the TTY
        os.system("stty sane")

def run_elf_test(filename):
    """Assembles and runs ELF binaries via GCC-Linker and QEMU-User."""
    cwd = os.path.join(TESTING_ROOT, ELF_DIR)
    obj_file = filename.replace('.s', '.o')
    exe_file = filename.replace('.s', '.elf_exe')
    test_file = filename.replace('.s', '.test')

    if not os.path.exists(os.path.join(cwd, test_file)): return

    try:
        # 1. Assemble
        subprocess.run([ASSEMBLER, "-f", "elf", "-q", "-o", obj_file, filename], cwd=cwd, check=True, capture_output=True)

        # 2. Link with GCC
        link_cmd = ["riscv64-unknown-elf-gcc", "-march=rv32i", "-mabi=ilp32", "-nostdlib", obj_file, "-o", exe_file]
        c_file = filename.replace('.s', '.c')
        if os.path.exists(os.path.join(cwd, c_file)): link_cmd.insert(4, c_file)
        subprocess.run(link_cmd, cwd=cwd, check=True, capture_output=True)

        # 3. Run QEMU-User
        # Passing 'hello' as a default argument for tests like args_test.s
        proc = subprocess.run(["qemu-riscv32", f"./{exe_file}", "hello"], cwd=cwd, capture_output=True, text=True)
        
        with open(os.path.join(cwd, test_file), "r") as f:
            expected = normalize_output(f.read())

        # Support both Exit Code tests and Stdout tests
        if expected.isdigit():
            if proc.returncode == int(expected): print_result(filename, True, f"(Exit: {proc.returncode})")
            else: print_result(filename, False, f"Exit code mismatch: Got {proc.returncode}, expected {expected}")
        else:
            actual = normalize_output(proc.stdout)
            if actual == expected: print_result(filename, True, "(Stdout Match)")
            else: print_result(filename, False, f"Output Mismatch\nExpected:\n{expected}\nActual:\n{actual}")

        # Cleanup
        for f in [obj_file, exe_file]:
            p = os.path.join(cwd, f)
            if os.path.exists(p): os.remove(p)

    except Exception as e:
        print_result(filename, False, f"Error: {e}")

def run_modular_linking_test():
    """Specific test for multi-file linking with custom linker script."""
    cwd = os.path.join(TESTING_ROOT, MODULAR_DIR)
    if not os.path.exists(cwd): return
    
    test_name = "Modular_Link_Test"
    print(f"\n\033[1m[ Modular Linking ({MODULAR_DIR}) ]\033[0m")
    
    try:
        # Assemble parts
        subprocess.run([ASSEMBLER, "-f", "elf", "-q", "-o", "kstdlib.o", "kstdlib.s"], cwd=cwd, check=True)
        subprocess.run([ASSEMBLER, "-f", "elf", "-q", "-o", "main.o", "main.s"], cwd=cwd, check=True)
        
        # Link using custom script
        link_cmd = ["riscv64-unknown-elf-ld", "-m", "elf32lriscv", "-T", "link.ld", "main.o", "kstdlib.o", "-o", "final.elf"]
        subprocess.run(link_cmd, cwd=cwd, check=True, capture_output=True)
        
        # Run
        proc = subprocess.run(["qemu-riscv32", "./final.elf"], cwd=cwd, capture_output=True, text=True)
        actual = normalize_output(proc.stdout)
        
        with open(os.path.join(cwd, "modular_test.test"), "r") as f:
            expected = normalize_output(f.read())
            
        if actual == expected:
            print_result(test_name, True, "(Modular Stdout Match)")
        else:
            print_result(test_name, False, f"Expected: {expected}\nActual: {actual}")

        # Cleanup
        for f in ["main.o", "kstdlib.o", "final.elf"]:
            p = os.path.join(cwd, f)
            if os.path.exists(p): os.remove(p)
    except Exception as e:
        print_result(test_name, False, f"Exception: {e}")

# --- Main Test Runner ---

if __name__ == "__main__":
    print("\n\033[1;36m================================================\033[0m")
    print("\033[1;36m        Kdex Automated Test Suite v1.1          \033[0m")
    print("\033[1;36m================================================\033[0m")

    # 1. Flat Binary Tests
    print("\n\033[1m[ Flat Binaries ]\033[0m")
    if os.path.exists(os.path.join(TESTING_ROOT, FLAT_DIR)):
        for f in sorted(os.listdir(os.path.join(TESTING_ROOT, FLAT_DIR))):
            if f.endswith(".s") and f not in SKIP_FILES:
                run_flat_test(f)

    # 2. ELF Tests
    print("\n\033[1m[ ELF C-Interop ]\033[0m")
    if os.path.exists(os.path.join(TESTING_ROOT, ELF_DIR)):
        for f in sorted(os.listdir(os.path.join(TESTING_ROOT, ELF_DIR))):
            if f.endswith(".s") and f not in SKIP_FILES:
                run_elf_test(f)

    # 3. Modular Linking Test
    run_modular_linking_test()

    # Final Summary
    total = PASSED_COUNT + FAILED_COUNT
    color = "\033[92m" if FAILED_COUNT == 0 else "\033[91m"
    print(f"\n Total: {total} | {color}Passed: {PASSED_COUNT}\033[0m | \033[91mFailed: {FAILED_COUNT}\033[0m")
    
    if FAILED_COUNT > 0:
        sys.exit(1)
