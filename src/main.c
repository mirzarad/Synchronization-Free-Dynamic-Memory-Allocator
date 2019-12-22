#include <stdio.h>
#include "sfmm.h"

// This main should be thought of as a test class
int main(int argc, char const *argv[]) {

    sf_mem_init();

    double* ptr = sf_malloc(sizeof(double)); // Request a pointer to a double (8 Bytes) --> should use 8 Bytes.

    *ptr = 320320320e-320; // stores some memory address
    sf_show_heap();

    double* ptr2 = sf_malloc(sizeof(double));
    *ptr2 = 2000;
    sf_show_heap();

    double* ptr3 = sf_malloc(sizeof(double));
    *ptr3 = 3000;
    sf_show_heap();

    char* ptr4 = sf_malloc(1000*sizeof(char));
    *ptr4 = 'O';
    sf_show_heap();

    char* ptr5 = sf_malloc(1000*sizeof(char));
    *ptr5 = 'O';
    sf_show_heap();

    char* ptr6 = sf_malloc(1000*sizeof(char));
    *ptr6 = 'O';
    sf_show_heap();

    char* ptr7 = sf_malloc(1000*sizeof(char));
    *ptr7 = 'O';
    sf_show_heap();

    printf("%f\n", *ptr);

    void* ptr8 = sf_malloc(20000*sizeof(char));
    printf("%p\n", ptr8);
    sf_show_heap();

    sf_mem_fini();

    return EXIT_SUCCESS;
}
