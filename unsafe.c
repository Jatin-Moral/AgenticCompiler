#include <stdio.h>
#include <string.h>
#include <stdlib.h> // <--- REQUIRED for system()

int main() {
    char buffer[10];
    
    // Dangerous: Buffer Overflow
    char *bad_string = "This string is definitely too long for the buffer";
    strcpy(buffer, bad_string);
    
    // Dangerous: Command Injection
    system("ls -la");

    return 0;
}
