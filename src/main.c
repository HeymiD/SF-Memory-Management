#include <stdio.h>
#include "sfmm.h"

int main(int argc, char const *argv[]) {
    sf_mem_init();

    double* a = sf_malloc(15*sizeof(double));


    *a=1;


    //sf_show_heap();

    double* pp = sf_realloc(a,10*sizeof(double));
    printf("%lf\n", *pp);

    sf_free(pp);

    sf_show_heap();


    sf_mem_fini();

    return EXIT_SUCCESS;
}
