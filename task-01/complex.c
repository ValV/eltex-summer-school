#include <stdio.h>
#include <tgmath.h>
#include <complex.h>

float complex cx1 = 0 + 0 * I, cx2 = 0 + 0 * I; // complex numbers
double complex cx = 0 + 0 * I; // complex result number

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
                cx = cx1 + cx2;
                printf("\t(%g%+gi)+(%g%+gi) = (%g%+g)+(%g%+g)i = %g%+gi\n\n",
                        creal(cx1), cimag(cx1), creal(cx2), cimag(cx2),
                        creal(cx1), creal(cx2), cimag(cx1), cimag(cx2),
                        creal(cx), cimag(cx));
		break;
            case 2:
                printf("\nSubtraction:\n");
                cx = cx1 - cx2;
                printf("\t(%g%+gi)-(%g%+gi) = (%g%+g)+(%g%+g)i = %g%+gi\n\n",
                        creal(cx1), cimag(cx1), creal(cx2), cimag(cx2),
                        creal(cx1), -creal(cx2), cimag(cx1), -cimag(cx2),
                        creal(cx), cimag(cx));
		break;
            case 3:
                printf("\nMultiplication...\n"); break;
            case 4:
                printf("\nDivision...\n"); break;
            case -1:
                printf("\nExit...\n"); break;
            default:
                printf("Unknown operation number!\n");
        }
    } while (opcode >= 0);
    return 0;
}
