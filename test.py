import os
import subprocess
import re

test_dir = "./tests"
# --- CONFIGURATION: FILES TO SKIP ---
SKIP_FILES = [
    "included.s",
    "big_macro.s",
]

# Get all .s files, excluding the ones in SKIP_FILES
test_files = [
    f for f in os.listdir(test_dir) 
    if f.endswith('.s') and f not in SKIP_FILES
]

# --- GLOBAL COUNTERS ---
PASSED_COUNT = 0
FAILED_COUNT = 0

def strip_ansi(text):
    ansi_escape = re.compile(r'\x1B(?:[@-Z\\-_]|\[[0-?]*[ -/]*[@-~])')
    return ansi_escape.sub('', text)

def normalize_output(text):
    text = strip_ansi(text)
    text = text.replace("./tests/", "tests/")
    text = re.sub(r':\d+\b', ':XX', text)
    # Remove "make: Leaving directory" lines
    lines = text.splitlines()
    lines = [line for line in lines if not line.startswith("make[")]
    return "\n".join(lines).strip()

def run_test(filename):
    file_path = os.path.join(test_dir, filename)
    test_file_path = os.path.join(test_dir, filename.replace('.s', '.test'))
    input_file_path = os.path.join(test_dir, filename.replace('.s', '.in'))
    
    if not os.path.exists(test_file_path):
        print(f" [?] {filename} [SKIPPED] - Missing {filename.replace('.s', '.test')}")
        return

    with open(test_file_path, "r") as f:
        expected_output = normalize_output(f.read().strip())

    input_data = None
    if os.path.exists(input_file_path):
        with open(input_file_path, "r") as f:
            input_data = f.read()

    is_error_test = "[ERROR]" in expected_output or "[WARN]" in expected_output

    command = ["make", "--no-print-directory", "run", f"FILE={file_path}"]
    
    full_output = "" 
    current_timeout = 5 if input_data else 1

    try:
        run_args = {
            "stdout": subprocess.PIPE,
            "stderr": subprocess.STDOUT,
            "text": True,
            "timeout": current_timeout
        }

        if input_data:
            run_args["input"] = input_data
        else:
            run_args["stdin"] = subprocess.DEVNULL

        proc = subprocess.run(command, **run_args)
        full_output = proc.stdout

    except subprocess.TimeoutExpired as e:
        partial_output = e.stdout if e.stdout else ""
        if isinstance(partial_output, bytes):
            partial_output = partial_output.decode('utf-8', errors='ignore')
            
        if "--- Running QEMU ---" in partial_output:
            runtime_part = partial_output.split("--- Running QEMU ---")[1].strip()
            if normalize_output(runtime_part) == expected_output:
                failed(filename, runtime_part + "\n[TIMEOUT - DID NOT EXIT]", expected_output)
            else:
                failed(filename, runtime_part + "\n[TIMEOUT - INCOMPLETE]", expected_output)
        else:
            failed(filename, partial_output + "\n[TIMEOUT - NO START]", expected_output)
        return

    except Exception as e:
        print(f" [!] Error running QEMU: {e}")
        return

    if full_output is None:
        full_output = ""
    elif isinstance(full_output, bytes):
        full_output = full_output.decode('utf-8', errors='ignore')

    # --- VERIFICATION LOGIC ---
    if is_error_test:
        start_marker = "--- Assembling ---"
        end_marker = "--- SYMBOL TABLE DUMP ---"

        if start_marker in full_output and end_marker in full_output:
            raw_log = full_output.split(start_marker)[1].split(end_marker)[0].strip()
            clean_log = normalize_output(raw_log)

            if clean_log == expected_output:
                passed(filename)
            else:
                failed(filename, clean_log, expected_output)
        else:
            failed(filename, normalize_output(full_output.strip()), expected_output)

    else:
        qemu_marker = "--- Running QEMU ---"
        
        if qemu_marker not in full_output:
            failed(filename, "QEMU did not start (Compilation Failed?)", expected_output)
            return

        runtime_output = full_output.split(qemu_marker)[1].strip()
        runtime_output = normalize_output(runtime_output)

        if runtime_output == expected_output:
            passed(filename)
        else:
            failed(filename, runtime_output, expected_output)

def passed(file):
    global PASSED_COUNT
    PASSED_COUNT += 1
    print(f" \033[92m{file} [PASSED]\033[0m")

def failed(file, actual, expected):
    global FAILED_COUNT
    FAILED_COUNT += 1
    print(f" \033[91m{file} [FAILED]\033[0m")
    print("-" * 20)
    print(f"Expected:\n{expected}")
    print("-" * 20)
    print(f"Actual:\n{actual}")
    print("-" * 20)

print(f"Running tests in {test_dir}...\n")

for file in test_files:
    run_test(file)

# --- SUMMARY STATISTICS ---
total_tests = PASSED_COUNT + FAILED_COUNT
if total_tests > 0:
    percentage = (PASSED_COUNT / total_tests) * 100
else:
    percentage = 0.0

print(f"\n\t{PASSED_COUNT} \033[92mPASSED\033[0m, {FAILED_COUNT} \033[91mFAILED\033[0m")
