/*
 * Data used in single-precision tan(x) function.
 *
 * Copyright (c) 2022-2023, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "math_config.h"

const struct tanf_poly_data __tanf_poly_data = {
.poly_tan = {
/* Coefficients generated using:
   remez(f(x) = (tan(sqrt(x)) - sqrt(x)) / (x * sqrt(x)), deg, [a;b], 1, 1e-16, [|dtype ...|])
   optimize each coefficient
   optimize relative error
   final prec : 23 bits
   working prec : 128 bits
   deg : 6
   a : 0x1p-126
   b : (pi) / 0x1p2
   dirty rel error : 0x1.df324p-26
   dirty abs error : 0x1.df3244p-26.  */
0x1.555558p-2, /* 0.3333334.  */
0x1.110e1cp-3, /* 0.1333277.  */
0x1.bb0e7p-5, /* 5.408403e-2.  */
0x1.5826c8p-6, /* 2.100534e-2.  */
0x1.8426a6p-7, /* 1.1845428e-2.  */
-0x1.7a5adcp-10, /* -1.4433095e-3.  */
0x1.5574dap-8, /* 5.210212e-3.  */
},
.poly_cotan = {
/* Coefficients generated using:
   fpminimax(f(x) = (0x1p0 / tan(sqrt(x)) - 0x1p0 / sqrt(x)) / sqrt(x), deg, [|dtype ...|], [a;b])
   optimize a single polynomial
   optimize absolute error
   final prec : 23 bits
   working prec : 128 bits
   deg : 3
   a : 0x1p-126
   b : (pi) / 0x1p2
   dirty rel error : 0x1.81298cp-25
   dirty abs error : 0x1.a8acf4p-25.  */
-0x1.55555p-2, /* -0.33333325.  */
-0x1.6c23e4p-6, /* -2.2225354e-2.  */
-0x1.12dbap-9, /* -2.0969994e-3.  */
-0x1.05a1c2p-12, /* -2.495116e-4.  */
}
};
