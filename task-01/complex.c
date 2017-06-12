#include <stdio.h>
#include <tgmath.h>
#include <complex.h>

float complex cx1 = 0 + 0 * I, cx2 = 0 + 0 * I; // complex numbers

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
        printf("\nChoose operation for complex numbers: %g%+gi and %g%+gi:\n",
                creal(cx1), cimag(cx1), creal(cx2), cimag(cx2));
        printf("%6d: change numbers\t %6d: exit program\n", 0, -1);
        printf("%6d: addition (+)\t %6d: division (/)\n", 1, 4);
        printf("%6d: subtraction (-)\t %6d: multiplication (*)\n", 2, 3);
        scanf("%d", &opcode);
        switch (opcode) {
            case 0:
                read_numbers(); break;
            case 1:
                printf("Addition...\n"); break;
            case 2:
                printf("Subtraction...\n"); break;
            case 3:
                printf("Multiplication...\n"); break;
            case 4:
                printf("Division...\n"); break;
            case -1:
                printf("Exit...\n"); break;
            default:
                printf("Unknown operation number!\n");
        }
    } while (opcode >= 0);
    return 0;
}
