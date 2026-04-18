// main.c
// 1. Import the Assembly function
extern int process_data(int* array, int length);

// 2. Import the Assembly data array
extern int multipliers[3];

int main() {
    // SECTION TEST: Overwrite the Assembly variable from C.
    // Assembly originally set multipliers[1] to 5. We change it to 10.
    multipliers[1] = 10; 

    // Create an array to sum (1 + 2 + 3 + 4 = 10)
    int my_array[] = {1, 2, 3, 4};
    int length = 4;

    // Call Assembly: 
    // It will sum the array (10) and multiply it by multipliers[1] (10)
    int result = process_data(my_array, length);

    // Return the result to QEMU!
    // Expected exit code: 100
    return result; 
}
