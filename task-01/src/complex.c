/* File         : complex.c
 * Description  : Simple complex number calculator
 */

#include <stdio.h>
#include <complex.h>

#include "funcscx.h"

void read_numbers() {
    float fx1r = 0, fx1i = 0, fx2r = 0, fx2i = 0; // temporary floats
    printf("Enter complex numbers 'x1 x2' (where x=a+bi, e.g. -3+8i):\n");
    while (scanf("%f%fi %f%fi", &fx1r, &fx1i, &fx2r, &fx2i) != 4) {
        // input errors (show hint)
        printf("Numbers must be: [-|+]a1<-|+>b1i [-|+]a2<-|+>b2i\n");
        // flush the line if any
        while (!feof(stdin) && fgetc(stdin) != '\n');
    }
    cx1 = fx1r + fx1i * I;
    cx2 = fx2r + fx2i * I;
}

int main(int argc, char *argv[]) {
    int opcode = 0;
    read_numbers(); // prompt for input
    do {
        // display numbers
        printf("\nChoose operation for complex numbers: %g%+gi and %g%+gi:\n",
                creal(cx1), cimag(cx1), creal(cx2), cimag(cx2));
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
                cx_addition();
		break;
            case 2:
                printf("\nSubtraction:\n");
                cx_subtraction();
		break;
            case 3:
                printf("\nMultiplication:\n");
                cx_multiplication();
		break;
            case 4:
                printf("\nDivision:\n");
                cx_division();
		break;
            case -1:
                printf("\nExit...\n"); break;
            default:
                printf("\nUnknown operation number!\n");
        }
    } while (opcode >= 0);
    return 0;
}
