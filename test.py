#!/usr/bin/env python3
import os
import subprocess
import re
import time

# --- Configuration ---
ASSEMBLER = os.path.abspath("./riscv-fasm") 
FLAT_DIR = "tests"
ELF_DIR = "ELFtest"
SKIP_FILES = ["included.s", "big_macro.s", "macros.s"] 

# --- Globals ---
PASSED_COUNT = 0
FAILED_COUNT = 0

# --- Helper Functions ---
def strip_ansi(text):
    ansi_escape = re.compile(r'\x1B(?:[@-Z\\-_]|\[[0-?]*[ -/]*[@-~])')
    return ansi_escape.sub('', text)

def normalize_output(text):
    text = strip_ansi(text)
    text = re.sub(r'[┌┐└┘│─]', '', text)
    text = text.replace(f"./{FLAT_DIR}/", f"{FLAT_DIR}/")
    text = re.sub(r':\d+\b', ':XX', text)
    lines = text.splitlines()
    lines = [line for line in lines if not line.startswith("make[") and line.strip()]
    return "\n".join(lines).strip()

def print_result(file, passed, details=""):
    global PASSED_COUNT, FAILED_COUNT
    if passed:
        PASSED_COUNT += 1
        print(f" \033[92m✔ {file} [PASSED]\033[0m {details}")
    else:
        FAILED_COUNT += 1
        print(f" \033[91m✘ {file} [FAILED]\033[0m")
        if details:
            print("-" * 40)
            print(details)
            print("-" * 40)

# --- Flat Binary Tests (Stdout Matching) ---
def run_flat_test(filename):
    file_path = os.path.join(FLAT_DIR, filename)
    test_file_path = os.path.join(FLAT_DIR, filename.replace('.s', '.test'))
    input_file_path = os.path.join(FLAT_DIR, filename.replace('.s', '.in'))
    
    if not os.path.exists(test_file_path):
        print(f" \033[93m? {filename} [SKIPPED - No .test file]\033[0m")
        return

    with open(test_file_path, "r") as f:
        expected_output = normalize_output(f.read().strip())

    input_data = ""
    if os.path.exists(input_file_path):
        with open(input_file_path, "r") as f:
            input_data = f.read().replace('\r\n', '\n')

    is_error_test = "[ERROR]" in expected_output or "[WARN]" in expected_output
    command = ["make", "--no-print-directory", "run", f"FILE={file_path}", "QUIET=1", "FORMAT=flat"]
    
    try:
        proc = subprocess.Popen(command, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)

        if input_data:
            for char in input_data:
                proc.stdin.write(char)
                proc.stdin.flush()
                time.sleep(0.01)
            
        try:
            full_output, _ = proc.communicate(timeout=10)
        except subprocess.TimeoutExpired:
            proc.kill()
            full_output, _ = proc.communicate()
            full_output += "\n[TIMEOUT]"

    except Exception as e:
        print_result(filename, False, f"Exception: {e}")
        return

    if is_error_test:
        clean_log = normalize_output(full_output.strip())
        if expected_output in clean_log:
            print_result(filename, True)
        else:
            print_result(filename, False, f"\033[1;33mExpected:\033[0m\n{expected_output}\n\n\033[1;31mActual:\033[0m\n{clean_log}")
    else:
        qemu_marker = "--- Running QEMU ---"
        if qemu_marker not in full_output:
            print_result(filename, False, f"QEMU did not start\n{normalize_output(full_output)}")
            return

        runtime_output = normalize_output(full_output.split(qemu_marker)[1].strip())
        if runtime_output == expected_output:
            print_result(filename, True)
        else:
            print_result(filename, False, f"\033[1;33mExpected:\033[0m\n{expected_output}\n\n\033[1;31mActual:\033[0m\n{runtime_output}")

# --- ELF C-Interop Tests (Exit Code Matching) ---
def run_elf_test(filename):
    obj_file = filename.replace('.s', '.o')
    exe_file = filename.replace('.s', '.elf')
    c_file = filename.replace('.s', '.c')
    c_path = os.path.join(ELF_DIR, c_file)
    
    test_file = os.path.join(ELF_DIR, filename.replace('.s', '.test'))
    expected_content = ""
    if os.path.exists(test_file):
        with open(test_file, "r") as f:
            expected_content = f.read().strip()

    try:
        # 1. Assemble
        asm_cmd = [ASSEMBLER, filename, "-f", "elf", "-q", "-o", obj_file]
        proc = subprocess.run(asm_cmd, capture_output=True, text=True, cwd=ELF_DIR)
        if proc.returncode != 0:
            print_result(filename, False, f"Assembler Failed:\n{proc.stderr}\n{proc.stdout}")
            return

        # 2. Link
        link_cmd = ["riscv64-unknown-elf-gcc", "-march=rv32i", "-mabi=ilp32", "-nostdlib"]
        if os.path.exists(c_path): link_cmd.append(c_file)
        link_cmd.extend([obj_file, "-o", exe_file])
        
        proc = subprocess.run(link_cmd, capture_output=True, text=True, cwd=ELF_DIR)
        if proc.returncode != 0:
            print_result(filename, False, f"GCC Linking Failed:\n{proc.stderr}")
            return

        # 3. Execute and Capture Output
        run_cmd = ["qemu-riscv32", f"./{exe_file}", "hello"]
        proc = subprocess.run(run_cmd, capture_output=True, text=True, cwd=ELF_DIR)
        actual_exit = proc.returncode
        actual_stdout = normalize_output(proc.stdout.strip())

        # Cleanup
        for f in [os.path.join(ELF_DIR, obj_file), os.path.join(ELF_DIR, exe_file)]:
            if os.path.exists(f): os.remove(f)

        # 4. Evaluation Logic
        # If expected is a number, check Exit Code. Otherwise, check Stdout.
        if expected_content.isdigit():
            expected_exit = int(expected_content)
            if actual_exit == expected_exit:
                print_result(filename, True, f"(Exit: {actual_exit})")
            else:
                print_result(filename, False, f"Expected Exit: {expected_exit}\nActual Exit: {actual_exit}")
        else:
            expected_stdout = normalize_output(expected_content)
            if actual_stdout == expected_stdout:
                print_result(filename, True, "(Stdout Match)")
            else:
                print_result(filename, False, f"Expected Stdout:\n{expected_stdout}\n\nActual Stdout:\n{actual_stdout}")

    except Exception as e:
        print_result(filename, False, f"Exception: {e}")

# --- Main Test Runner ---
if __name__ == "__main__":
    print("\n\033[1;36m================================================\033[0m")
    print("\033[1;36m       Kdex Automated Test Suite v1.0           \033[0m")
    print("\033[1;36m================================================\033[0m")

    print("\n\033[1m[ Flat Binaries (tests/) ]\033[0m")
    if os.path.exists(FLAT_DIR):
        flat_files = [f for f in os.listdir(FLAT_DIR) if f.endswith('.s') and f not in SKIP_FILES]
        for f in sorted(flat_files):
            run_flat_test(f)
    else:
        print(f" Directory '{FLAT_DIR}' not found.")

    print("\n\033[1m[ Relocatable ELFs & C-Interop (ELFtest/) ]\033[0m")
    if os.path.exists(ELF_DIR):
        elf_files = [f for f in os.listdir(ELF_DIR) if f.endswith('.s') and f not in SKIP_FILES]
        for f in sorted(elf_files):
            run_elf_test(f)
    else:
        print(f" Directory '{ELF_DIR}' not found.")

    print("\n\033[1;36m------------------------------------------------\033[0m")
    total = PASSED_COUNT + FAILED_COUNT
    color = "\033[92m" if FAILED_COUNT == 0 else "\033[91m"
    print(f" Total: {total} | {color}Passed: {PASSED_COUNT}\033[0m | \033[91mFailed: {FAILED_COUNT}\033[0m")
    print("\033[1;36m================================================\033[0m\n")
    
    if FAILED_COUNT > 0:
        exit(1)
