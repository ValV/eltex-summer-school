/* File		: funcscx.h
 * Description	: Header file for funcscx.c
 */

#ifndef FUNCSCX_H
#define FUNCSCX_H

#include <complex.h>

// shared variables
extern float complex cx1;
extern float complex cx2;

// function prototypes
void cx_addition();
void cx_subtraction();
void cx_multiplication();
void cx_division();

#endif /* FUNCSCX_H */
