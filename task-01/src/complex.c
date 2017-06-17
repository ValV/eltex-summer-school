/* File         : complex.c
 * Description  : Simple complex number calculator
 */

#include <stdio.h>
#include <dlfcn.h>

#include "funcscx.h"

float fx1r = 0, fx1i = 0, fx2r = 0, fx2i = 0; // temporary floats

void read_numbers() {
    printf("Enter complex numbers 'x1 x2' (where x=a+bi, e.g. -3+8i):\n");
    while (scanf("%f%fi %f%fi", &fx1r, &fx1i, &fx2r, &fx2i) != 4) {
        // input errors (show hint)
        printf("Numbers must be: [-|+]a1<-|+>b1i [-|+]a2<-|+>b2i\n");
        // flush the line if any
        while (!feof(stdin) && fgetc(stdin) != '\n');
    }
}

int main(int argc, char *argv[]) {
    // initialize library handlers
    void *dl_addition = NULL;
    void *dl_subtraction = NULL;
    void *dl_multiplication = NULL;
    void *dl_division = NULL;

    // initialize function handlers
    cx_func_t cx_addition = NULL;
    cx_func_t cx_subtraction = NULL;
    cx_func_t cx_multiplication = NULL;
    cx_func_t cx_division = NULL;

    char *error;

    int opcode = 0;

    // load addition module
    dl_addition = dlopen("libcxadd.so", RTLD_NOW);
    if (!dl_addition)
        fputs(dlerror(), stderr);
    else {
        cx_addition = (cx_func_t) dlsym(dl_addition, "cx_addition");
        if ((error = dlerror()) != NULL)
            fputs(error, stderr);
    }
    // load subtraction module
    dl_subtraction = dlopen("libcxsub.so", RTLD_NOW);
    if (!dl_subtraction)
        fputs(dlerror(), stderr);
    else {
        cx_subtraction = (cx_func_t) dlsym(dl_subtraction, "cx_subtraction");
        if ((error = dlerror()) != NULL)
            fputs(error, stderr);
    }
    // load multiplication module
    dl_multiplication = dlopen("libcxmul.so", RTLD_NOW);
    if (!dl_multiplication)
        fputs(dlerror(), stderr);
    else {
        cx_multiplication = (cx_func_t) dlsym(dl_multiplication,
                "cx_multiplication");
        if ((error = dlerror()) != NULL)
            fputs(error, stderr);
    }
    // load division module
    dl_division = dlopen("libcxdiv.so", RTLD_NOW);
    if (!dl_division)
        fputs(dlerror(), stderr);
    else {
        cx_division = (cx_func_t) dlsym(dl_division, "cx_division");
        if ((error = dlerror()) != NULL)
            fputs(error, stderr);
    }
    read_numbers(); // prompt for input
    do {
        // display numbers
        printf("\nChoose operation for complex numbers: %g%+gi and %g%+gi:\n",
                fx1r, fx1i, fx2r, fx2i);
        // display menu
        printf("\t%4d: change numbers\t %4d: exit program\n\n", 0, -1);
        printf("\t%4d: addition (+)\t %4d: multiplication (*)\n", 1, 3);
        printf("\t%4d: subtraction (-)\t %4d: division (/)\n", 2, 4);
        scanf("%d", &opcode);
        // process menu entry
        switch (opcode) {
            case 0:
                read_numbers(); break;
            case 1:
                printf("\nAddition:\n");
                if (cx_addition)
                    (*cx_addition)(fx1r, fx1i, fx2r, fx2i);
                else
                    puts("Operation is not available");
		break;
            case 2:
                printf("\nSubtraction:\n");
                if (cx_subtraction)
                    (*cx_subtraction)(fx1r, fx1i, fx2r, fx2i);
                else
                    puts("Operation is not available");
		break;
            case 3:
                printf("\nMultiplication:\n");
                if (cx_multiplication)
                    (*cx_multiplication)(fx1r, fx1i, fx2r, fx2i);
                else
                    puts("Operation is not available");
		break;
            case 4:
                printf("\nDivision:\n");
                if (cx_division)
                    (*cx_division)(fx1r, fx1i, fx2r, fx2i);
                else
                    puts("Operation is not available");
		break;
            case -1:
                printf("\nExit...\n"); break;
            default:
                printf("\nUnknown operation number!\n");
        }
    } while (opcode >= 0);
    return 0;
}
