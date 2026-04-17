import os
import subprocess
import re
import time

test_dir = "./tests"
SKIP_FILES = ["included.s", "big_macro.s"]

test_files = [
    f for f in os.listdir(test_dir) 
    if f.endswith('.s') and f not in SKIP_FILES
]

PASSED_COUNT = 0
FAILED_COUNT = 0

def strip_ansi(text):
    ansi_escape = re.compile(r'\x1B(?:[@-Z\\-_]|\[[0-?]*[ -/]*[@-~])')
    return ansi_escape.sub('', text)

def normalize_output(text):
    text = strip_ansi(text)
    text = re.sub(r'[┌┐└┘│─]', '', text)
    text = text.replace("./tests/", "tests/")
    text = re.sub(r':\d+\b', ':XX', text)
    lines = text.splitlines()
    lines = [line for line in lines if not line.startswith("make[") and line.strip()]
    return "\n".join(lines).strip()

def run_test(filename):
    file_path = os.path.join(test_dir, filename)
    test_file_path = os.path.join(test_dir, filename.replace('.s', '.test'))
    input_file_path = os.path.join(test_dir, filename.replace('.s', '.in'))
    
    if not os.path.exists(test_file_path):
        print(f" [?] {filename} [SKIPPED]")
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
        # Start the process with a pipe for stdin
        proc = subprocess.Popen(
            command,
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True
        )

        if input_data:
            # Trickle the input: send 1 character at a time with a tiny delay
            # This prevents UART FIFO overflows in QEMU
            for char in input_data:
                proc.stdin.write(char)
                proc.stdin.flush()
                time.sleep(0.01) # 10ms delay between keys
            
        # Capture the output with a strict timeout
        try:
            full_output, _ = proc.communicate(timeout=10)
        except subprocess.TimeoutExpired:
            proc.kill()
            full_output, _ = proc.communicate()
            full_output += "\n[TIMEOUT]"

    except Exception as e:
        print(f" [!] Error: {e}")
        return

    # --- VERIFICATION LOGIC ---
    if is_error_test:
        clean_log = normalize_output(full_output.strip())
        if clean_output_matches(clean_log, expected_output):
            passed(filename)
        else:
            failed(filename, clean_log, expected_output)
    else:
        qemu_marker = "--- Running QEMU ---"
        if qemu_marker not in full_output:
            failed(filename, "QEMU did not start\n" + normalize_output(full_output), expected_output)
            return

        runtime_output = full_output.split(qemu_marker)[1].strip()
        runtime_output = normalize_output(runtime_output)

        if runtime_output == expected_output:
            passed(filename)
        else:
            failed(filename, runtime_output, expected_output)

def clean_output_matches(actual, expected):
    # For error tests, allow the expected error to be a substring 
    # since UI banners might persist
    return expected in actual

def passed(file):
    global PASSED_COUNT
    PASSED_COUNT += 1
    print(f" \033[92m✔ {file} [PASSED]\033[0m")

def failed(file, actual, expected):
    global FAILED_COUNT
    FAILED_COUNT += 1
    print(f" \033[91m✘ {file} [FAILED]\033[0m")
    print("-" * 30)
    print(f"\033[1;33mExpected:\033[0m\n{expected}")
    print("-" * 30)
    print(f"\033[1;31mActual:\033[0m\n{actual}")
    print("-" * 30)

print("\033[1mStarting RISC-V-FASM Automated Test Suite\033[0m")
print(f"Scanning {test_dir}...\n")

for file in test_files:
    run_test(file)

print(f"\nFinal Result: {PASSED_COUNT} \033[92mPASSED\033[0m, {FAILED_COUNT} \033[91mFAILED\033[0m")
