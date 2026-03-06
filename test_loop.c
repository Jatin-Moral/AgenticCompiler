#include <stdio.h>

int main() {
    volatile long long i;
    for (i = 0; i < 500000000; i++);  // busy loop
    printf("Finished loop\n");
    return 0;
}
