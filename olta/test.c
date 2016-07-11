#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

int main(int argc, char *argv[]) {
    unsigned int r = atoi(argv[1]);

    fprintf(stderr, "%d\n", (32 - __builtin_clz(r))); 

    return 0;
}
