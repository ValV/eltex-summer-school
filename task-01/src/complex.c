/* File         : complex.c
 * Description  : Simple complex number calculator
 */

#include <stdio.h>
#include <dlfcn.h>

#include "funcscx.h"

struct module {
    char *filename;
    void *handle;
    char *symbol;
    cx_func_t function;
}; // a structure to hold loadable module's params

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
    // fill in the array of modules (plugins)
    struct module plugins[4] = {
        {.filename = "libcxadd.so", .handle = NULL,
            .symbol = "cx_addition", .function = NULL},
        {.filename = "libcxsub.so", .handle = NULL,
            .symbol = "cx_subtraction", .function = NULL},
        {.filename = "libcxmul.so", .handle = NULL,
            .symbol = "cx_multiplication", .function = NULL},
        {.filename = "libcxdiv.so", .handle = NULL,
            .symbol = "cx_division", .function = NULL}};

    char *error;

    int opcode = 0;

    // sequentially load modules (plugins)
    for (int i = 0; i < 4; i ++) {
        plugins[i].handle = dlopen(plugins[i].filename, RTLD_NOW);
        if (plugins[i].handle) {
            plugins[i].function =
                (cx_func_t) dlsym(plugins[i].handle, plugins[i].symbol);
            if ((error = dlerror()) != NULL)
                fprintf(stderr, "%s\n", error);
        }
        else
            fprintf(stderr, "%s\n", dlerror());
    }

    read_numbers(); // prompt for input
    do {
        // display numbers
        printf("\nChoose operation for complex numbers: %g%+gi and %g%+gi:\n",
                fx1r, fx1i, fx2r, fx2i);
        // display menu
        printf("\t%4d: %15.15s\t%4d: %15.15s\n\n",
                -1, "exit program", 0, "change numbers");
        // build entries upon modules available
        for (int i = 0, j = 0; i < 4; i ++) {
            if (plugins[i].function) {
                printf("\t%4d: %15.15s", i + 1, (plugins[i].symbol + 3));
                if ((j ++) & 1 || i == 3) printf("\n");
            }
        }
        scanf("%d", &opcode);
        // process menu entry
        switch (opcode) {
            case 0:
                read_numbers(); break;
            case 1:
                printf("\nAddition:\n");
                if (plugins[opcode - 1].function)
                    (*plugins[opcode - 1].function)(fx1r, fx1i, fx2r, fx2i);
                else
                    puts("Operation is not available");
		break;
            case 2:
                printf("\nSubtraction:\n");
                if (plugins[opcode - 1].function)
                    (*plugins[opcode - 1].function)(fx1r, fx1i, fx2r, fx2i);
                else
                    puts("Operation is not available");
		break;
            case 3:
                printf("\nMultiplication:\n");
                if (plugins[opcode - 1].function)
                    (*plugins[opcode - 1].function)(fx1r, fx1i, fx2r, fx2i);
                else
                    puts("Operation is not available");
		break;
            case 4:
                printf("\nDivision:\n");
                if (plugins[opcode - 1].function)
                    (*plugins[opcode - 1].function)(fx1r, fx1i, fx2r, fx2i);
                else
                    puts("Operation is not available");
		break;
            case -1:
                printf("\nExit...\n"); break;
            default:
                printf("\nUnknown operation number!\n");
        }
    } while (opcode >= 0);

    for (int i = 0; i < 4; i ++) {
        if (plugins[i].handle) dlclose(plugins[i].handle);
    }

    return 0;
}
