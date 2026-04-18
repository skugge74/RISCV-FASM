extern int add_to_shared_var(int a);
extern int shared_var[2];
extern int compute_offset();

int main() {
    shared_var[1] = 20;

    // Test 1: la with addend (existing)
    int result1 = add_to_shared_var(22);  // expects 42

    // Test 2: li with compile-time math expression
    int result2 = compute_offset();       // expects 40

    if (result1 != 42) return 1;
    if (result2 != 40) return 2;
    return 42;
}
