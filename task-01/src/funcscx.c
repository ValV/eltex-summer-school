/* File		: funcscx.c
 * Description	: Implements complex number functions for base arithmetic
 */

#include <stdio.h>
#include <tgmath.h>
#include <complex.h>

#include "funcscx.h"

float complex cx1 = 0 + 0 * I, cx2 = 0 + 0 * I; // complex numbers
double complex cx = 0 + 0 * I; // complex result number

void cx_addition() {
    cx = cx1 + cx2;
    printf("\t(%g%+gi)+(%g%+gi) = (%g%+g)+(%g%+g)i = %g%+gi\n\n",
            creal(cx1), cimag(cx1), creal(cx2), cimag(cx2),
            creal(cx1), creal(cx2), cimag(cx1), cimag(cx2),
            creal(cx), cimag(cx));
}

void cx_subtraction() {
    cx = cx1 - cx2;
    printf("\t(%g%+gi)-(%g%+gi) = (%g%+g)+(%g%+g)i = %g%+gi\n\n",
            creal(cx1), cimag(cx1), creal(cx2), cimag(cx2),
            creal(cx1), -creal(cx2), cimag(cx1), -cimag(cx2),
            creal(cx), cimag(cx));
}

void cx_multiplication() {
    cx = cx1 * cx2;
    printf("\t(%g%+gi)*(%g%+gi) = ",
            creal(cx1), cimag(cx1), creal(cx2), cimag(cx2));
    printf("((%g*%g)-(%g*%g))+((%g*%g)+(%g*%g))i = ",
            creal(cx1), creal(cx2), cimag(cx1), cimag(cx2),
            cimag(cx1), creal(cx2), creal(cx1), cimag(cx2));
    printf("%g%+gi\n\n", creal(cx), cimag(cx));
}

void cx_division() {
    cx = cx1 / cx2;
    printf("\t(%g%+gi)/(%g%+gi) = ",
            creal(cx1), cimag(cx1), creal(cx2), cimag(cx2));
    printf("(((%g*%g)+(%g*%g))/((%g*%g)+(%g*%g)))+\n",
            creal(cx1), creal(cx2), cimag(cx1), cimag(cx2),
            creal(cx2), creal(cx2), cimag(cx2), cimag(cx2));
    printf("\t+(((%g*%g)-(%g*%g))/((%g*%g)+(%g*%g)))i = ",
            cimag(cx1), creal(cx2), creal(cx1), cimag(cx2),
            creal(cx2), creal(cx2), cimag(cx2), cimag(cx2));
    printf("%g%+gi\n\n", creal(cx), cimag(cx));
}
