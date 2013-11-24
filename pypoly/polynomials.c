#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "polynomials.h"

/**
 * Generic helpers
 */

#define MAX(a,b)    (((a)>(b))?(a):(b))

/* strdup is not part of the C standard and might not be available */
static inline char *__strdup(const char *s) {
    int len = strlen(s) + 1;
    char *p;
    if ((p = malloc(len * sizeof(char))) == NULL) {
        return NULL;
    }
    return memcpy(p, s, len);
}
#define strdup __strdup

/**
 * Complex numbers
 */

const Complex CZero = {0., 0.}, COne = {1., 0.};

/* Check if a complex number equals (0,0).
 * We ignore double precision related errors which shouldn't be a problem
 * in common use cases. */
static inline int complex_iszero(Complex c) {
    return (c.real == 0. && c.imag == 0.);
}

#ifndef PYPOLY_VERSION
static inline Complex complex_add(Complex a, Complex b) {
    return (Complex){
        a.real + b.real,
        a.imag + b.imag
    };
}
static inline Complex complex_sub(Complex a, Complex b) {
    return (Complex){
        a.real - b.real,
        a.imag - b.imag
    };
}
static inline Complex complex_neg(Complex z) {
    return (Complex){
        - z.real,
        - z.imag
    };
}
static inline Complex complex_mult(Complex a, Complex b) {
    return (Complex){
        a.real * b.real - a.imag * b.imag,
        a.real * b.imag + a.imag * b.real
    };
}
static inline Complex complex_div(Complex a, Complex b) {
    Complex r;
    double d = b.real*b.real + b.imag*b.imag;
    if (d == 0.) {
        errno = EDOM;
    } else {
        r.real = (a.real*b.real + a.imag*b.imag)/d;
        r.imag = (a.imag*b.real - a.real*b.imag)/d;
    }
    return r;
}
#else
/* cPython comes with good implementations of operations on complex numbers.
 * Let's use them! */
#define complex_add _Py_c_sum
#define complex_sub _Py_c_diff
#define complex_neg _Py_c_neg
#define complex_mult _Py_c_prod
#define complex_div _Py_c_quot
#endif

/**
 * Polynomials
 */

/* This macro recomputes the degree of the Polynomial pointed by P.
 * To be called after some operation modifying the leading coefficient. */
#define Poly_Resize(P)                                                  \
    while ((P)->deg != -1 && complex_iszero((P)->coef[(P)->deg])) {     \
        --((P)->deg);                                                   \
    }

/* Create a Polynomial of degree "deg" at address pointed by P.
 * If "deg" is -1, no memory is allocated and the coefficients pointer
 * is set to NULL. */
int poly_init(Polynomial *P, int deg) {
    if (deg == -1) {
        P->coef = NULL;
    } else if ((P->coef = calloc(deg + 1, sizeof(Complex))) == NULL) {
        P->coef = NULL;
        return 0;
    }
    P->deg = deg;
    return 1;
}

/* Free memory allocated for a polynomial */
void poly_free(Polynomial *P) {
    free(P->coef);
    P->coef = NULL;
}

int poly_initX(Polynomial *P) {
    if (!poly_init(P, 1)) {
        return 0;
    }
    poly_set_coef(P, 1, COne);
    return 1;
}

int poly_equal(Polynomial *P, Polynomial *Q) {
    if (P == Q) return 1;
    if (P->deg != Q->deg) return 0;
    int i;
    for (i = 0; i <= P->deg; ++i) {
        if (P->coef[i].real != Q->coef[i].real
                ||
            P->coef[i].imag != Q->coef[i].imag) {
            return 0;
        }
    }
    return 1;
}

/* Polynomials string representation.
 * Examples:
 *      -1 + 3 * X**2
 *      -1+2.5j + (1+3j) * X
 * We traverse the coefficients and append characters to a buffer.
 * A little bit messy, but it seems to work.
 */
#define BUFFER_AVAILABLE(buffer, offset)    \
    (sizeof(buffer)>(int)(offset))?         \
    (sizeof(buffer)-(int)(offset))          \
    : 0

#define STR_UNKOWN              "X"
#define STR_J                   "j"

char* poly_to_string(Polynomial *P) {
    if (P->deg == -1) {
        return "0";
    } else {
        char buffer[2048] = "";
        int i, multiplier, add_mult_sign, offset = 0;
        double re, im;
        for (i = 0; i <= P->deg; ++i) {
            if (complex_iszero(P->coef[i])) {
                continue;
            }

            multiplier = 1;
            add_mult_sign = 1;
            if (offset != 0) {
                multiplier = (P->coef[i].real <= 0 && P->coef[i].imag <= 0) ? -1 : 1;
                offset += snprintf(buffer + offset,
                                   BUFFER_AVAILABLE(buffer, offset),
                                   "%s", (multiplier == 1) ? " + " : " - ");
            }
            re = multiplier * P->coef[i].real;
            im = multiplier * P->coef[i].imag;
            if (P->coef[i].real == 0) {
                if (P->coef[i].imag != 1) {
                    offset += snprintf(buffer + offset,
                                       BUFFER_AVAILABLE(buffer, offset),
                                       "%g", im);
                }
                offset += snprintf(buffer + offset,
                                   BUFFER_AVAILABLE(buffer, offset),
                                   "%s", STR_J);
            } else if (im == 0) {
                if (re != 1 || i == 0) {
                    offset += snprintf(buffer + offset,
                                       BUFFER_AVAILABLE(buffer, offset),
                                       "%g", re);
                } else {
                    add_mult_sign = 0;
                }
            } else {
                offset += snprintf(buffer + offset,
                                   BUFFER_AVAILABLE(buffer, offset),
                                   i == 0 ? "%g%+g%s" : "(%g%+g%s)",
                                   re, im, STR_J);
            }
            if (i == 1) {
                offset += snprintf(buffer + offset,
                                   BUFFER_AVAILABLE(buffer, offset),
                                   "%s", add_mult_sign ? " * " STR_UNKOWN : STR_UNKOWN);
            } else if (i > 1) {
                offset += snprintf(buffer + offset,
                                   BUFFER_AVAILABLE(buffer, offset),
                                   "%s**%d", add_mult_sign ? " * " STR_UNKOWN : STR_UNKOWN, i);
            }
            if (offset >= sizeof(buffer)) {
                break;
            }
        }
        return strdup(buffer);
    }
}

void poly_set_coef(Polynomial *P, int i, Complex c) {
    /* /!\ i should be <= allocated */
    P->coef[i] = c;
    if (i > P->deg && !complex_iszero(c)) {
        P->deg = i;
    } else {
        Poly_Resize(P);
    }
}


/* Polynomial evaluation at a given point using Horner's method.
 * Performs O(deg P) operations (naïve approach is quadratic).
 * See http://en.wikipedia.org/wiki/Horner%27s_method */
Complex poly_eval(Polynomial *P, Complex c) {
    Complex result = CZero;
    int i;
    for (i = P->deg; i >= 0; --i) {
        result = complex_add(complex_mult(result, c), P->coef[i]);
    }
    return result;
}

/**
 * Polynomial operators
 * We use the following naming convention:
 *  A, B, C, ... for the operators parameters
 *  P, Q, R, ... for the result destination
 *
 * The operators should assume the destination polynomials were NOT initiliazed.
 * It is up to the operators to perform this initialization when relevant.
 */

/* Copy polynomial pointed by A to the location pointed by P */
int poly_copy(Polynomial *A, Polynomial *P) {
    if (!poly_init(P, A->deg)) {
        return 0;
    }
    memcpy(P->coef, A->coef, (A->deg + 1) * sizeof(Complex));
    return 1;
}

int poly_add(Polynomial *A, Polynomial *B, Polynomial *R) {
    if (!poly_init(R, MAX(A->deg, B->deg))) {
        return 0;
    }
    int i;
    for (i = 0; i <= MAX(A->deg, B->deg); ++i) {
        R->coef[i] = complex_add(Poly_GetCoef(A, i), Poly_GetCoef(B, i));
    }
    Poly_Resize(R);
    return 1;
}

int poly_sub(Polynomial *A, Polynomial *B, Polynomial *R) {
    if (!poly_init(R, MAX(A->deg, B->deg))) {
        return 0;
    }
    int i;
    for (i = 0; i <= MAX(A->deg, B->deg); ++i) {
        R->coef[i] = complex_sub(Poly_GetCoef(A, i), Poly_GetCoef(B, i));
    }
    Poly_Resize(R);
    return 1;
}

int poly_neg(Polynomial *A, Polynomial *Q) {
    if (!poly_init(Q, A->deg)) {
        return 0;
    }
    int i;
    for (i = 0; i <= A->deg; ++i) {
        Q->coef[i] = complex_neg(A->coef[i]);
    }
    return 1;
}

int poly_multiply(Polynomial *A, Polynomial *B, Polynomial *R) {
    // TODO: implement faster algo
    if (!poly_init(R, A->deg + B->deg)) {
        return 0;
    }
    int i, j;
    Complex tmp;
    for (i = 0; i <= A->deg + B->deg; ++i) {
        for (j = 0; j <= i; ++j) {
            tmp = complex_mult(Poly_GetCoef(A, j), Poly_GetCoef(B, i - j));
            (R->coef[i]).real += tmp.real;
            (R->coef[i]).imag += tmp.imag;
        }
    }
    return 1;
}

int poly_pow(Polynomial *A, unsigned int n, Polynomial *R) {
    // As for multiplication, we can do much better.
    int failure = 0;
    if (n == 0) {
        Poly_InitConst(R, ((Complex){1, 0}), failure)
        return !failure;
    }
    if (poly_copy(A, R)) {
        Polynomial T;
        while (--n > 0) {
            failure = !poly_multiply(R, A, &T);
            poly_free(R);
            if (failure) {
                return 0;
            }
            *R = T;
        }
        return 1;
    } else {
        return 0;
    }
}

int poly_derive(Polynomial *A, Polynomial *R) {
    if (!poly_init(R, MAX(-1, A->deg - 1))) {
        return 0;
    }
    int i;
    for (i = 0; i <= A->deg - 1; ++i) {
        R->coef[i] = complex_mult((Complex){i + 1, 0}, A->coef[i + 1]);
    }
    return 1;
}

/* Euclidean division of A by B.
 * If B is not zero, the resulting polynomials Q and R are defined by:
 *      A = B * Q + R, deg R < deg B
 * If B is zero, the operation is undefined and returns -1.
 */
int poly_div(Polynomial *A, Polynomial *B, Polynomial *Q, Polynomial *R) {
    if (B->deg == -1) {
        return -1;  // Division by zero
    }
    Complex B_leadcoef = Poly_LeadCoef(B);
    Polynomial T1, T2;  // Used as buffers

    /* Polynomials coefficients MUST be initialized. */
    if (Q != NULL) {
        poly_init(Q, -1);
    }
    poly_init(R, -1);
    poly_init(&T1, -1);
    poly_init(&T2, -1);

    if (Q != NULL && !poly_init(Q, A->deg)) goto error;
    if (!poly_copy(A, R)) goto error;

    while (R->deg - B->deg >= 0) {
        if (!poly_init(&T1, R->deg - B->deg)) goto error;
        poly_set_coef(&T1, R->deg - B->deg,
                      complex_div(Poly_LeadCoef(R), B_leadcoef));

        if (Q != NULL) {
            if (!poly_add(Q, &T1, &T2)) goto error;
            poly_free(Q);
            if (!poly_copy(&T2, Q)) goto error;
            poly_free(&T2);
        }

        if (!poly_multiply(&T1, B, &T2)) goto error;
        poly_free(&T1);
        if (!poly_sub(R, &T2, &T1)) goto error;
        poly_free(&T2);
        poly_free(R);
        if(!poly_copy(&T1, R)) goto error;
        poly_free(&T1);
    }
    return 1;
error:
    if (Q != NULL && Q->coef != NULL) poly_free(Q);
    if (R->coef != NULL) poly_free(R);
    if (T1.coef != NULL) poly_free(&T1);
    if (T2.coef != NULL) poly_free(&T2);
    return 0;
}

int main() {
    Polynomial A, B, Q, R;
    int er;
    Poly_InitConst(&B, ((Complex){1, 0}), er)
    poly_init(&A, 1);
    //poly_init(&B, 5);
    int i;
    poly_set_coef(&A, 1, (Complex){1, 0});
    //for (i = 0; i <= 5; ++i)
    //    poly_set_coef(&B, i, (Complex){1, 0});
    printf("Num: %d, %s\n", A.deg, poly_to_string(&A));
    printf("Den: %d, %s\n", B.deg, poly_to_string(&B));
    poly_div(&A, &B, &Q, &R);
    printf("Quotient: %d, %s\n", Q.deg, poly_to_string(&Q));
    printf("Remainder: %d, %s\n", R.deg, poly_to_string(&R));
    return 0;
}