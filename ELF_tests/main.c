// Import the function and the variable from the Assembler!
extern int add_to_shared_var(int a);
extern int shared_var;

int main() {
    // 1. Try to overwrite the Assembly variable from C.
    // If the Assembler failed to separate .data and .text, 
    // this exact line will cause a Segmentation Fault.
    shared_var = 20;
    
    // 2. Call the Assembly function to add 22 to our variable.
    // It will load 20, add 22, store 42, and return 42.
    int result = add_to_shared_var(22);
    
    // Return 42 to the OS
    return result; 
}
