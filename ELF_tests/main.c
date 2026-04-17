// Tell C our assembly function exists
extern int add_numbers(int a, int b);

int main() {
    int result = add_numbers(15, 27);
    
    // The return value of main becomes the program's exit code
    return result; 
}
