Simple complex number calculator
====

This README in other languages: [Russian](README.ru-RU.md)

This program implements four basic arithmetic operations with complex numbers: addition, subtraction, multiplication, and division. Accepts two complex numbers formatted as:

    [+|-]a<+|->bi [+|-]c<+|->di

Here the sign before imaginary part is mandatory, as well as imaginary unit at the end (letter *i*).

The program uses *complex.h* from *C99* standard to work with complex numbers (declaration, initialization, and output). It also uses *tgmath.h* (from the same *C99*) for convenience (e.g. call *creal()* for both *float complex* and *double complex* arguments).

To build (assuming that *gcc* is properly installed in the system) and run execute from command line:

    $ make && ./complex
