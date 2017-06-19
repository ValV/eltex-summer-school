/* File		: funcscx.c
 * Description	: Implements complex number functions for base arithmetic
 */

#include <stdio.h>
#include <tgmath.h>
#include <complex.h>

#include "funcscx.h"

static float complex cx1 = 0 + 0 * I, cx2 = 0 + 0 * I; // complex numbers
static double complex cx = 0 + 0 * I; // complex result number

cx_func_t cx_addition(float fx1r, float fx1i, float fx2r, float fx2i) {
    cx1 = fx1r + fx1i * I;
    cx2 = fx2r + fx2i * I;
    cx = cx1 + cx2;
    printf("\t(%g%+gi)+(%g%+gi) = (%g%+g)+(%g%+g)i = %g%+gi\n\n",
            creal(cx1), cimag(cx1), creal(cx2), cimag(cx2),
            creal(cx1), creal(cx2), cimag(cx1), cimag(cx2),
            creal(cx), cimag(cx));
}

cx_func_t cx_subtraction(float fx1r, float fx1i, float fx2r, float fx2i) {
    cx1 = fx1r + fx1i * I;
    cx2 = fx2r + fx2i * I;
    cx = cx1 - cx2;
    printf("\t(%g%+gi)-(%g%+gi) = (%g%+g)+(%g%+g)i = %g%+gi\n\n",
            creal(cx1), cimag(cx1), creal(cx2), cimag(cx2),
            creal(cx1), -creal(cx2), cimag(cx1), -cimag(cx2),
            creal(cx), cimag(cx));
}

cx_func_t cx_multiplication(float fx1r, float fx1i, float fx2r, float fx2i) {
    cx1 = fx1r + fx1i * I;
    cx2 = fx2r + fx2i * I;
    cx = cx1 * cx2;
    printf("\t(%g%+gi)*(%g%+gi) = ",
            creal(cx1), cimag(cx1), creal(cx2), cimag(cx2));
    printf("((%g*%g)-(%g*%g))+((%g*%g)+(%g*%g))i = ",
            creal(cx1), creal(cx2), cimag(cx1), cimag(cx2),
            cimag(cx1), creal(cx2), creal(cx1), cimag(cx2));
    printf("%g%+gi\n\n", creal(cx), cimag(cx));
}

cx_func_t cx_division(float fx1r, float fx1i, float fx2r, float fx2i) {
    cx1 = fx1r + fx1i * I;
    cx2 = fx2r + fx2i * I;
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
