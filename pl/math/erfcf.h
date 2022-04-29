/*
 * Shared functions for scalar and vector single-precision erfc(x) functions.
 *
 * Copyright (c) 2021-2022, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#ifndef PL_MATH_ERFCF_H
#define PL_MATH_ERFCF_H

#include <math.h>

/* Accurate exponential from optimized-routines.  */
double
__exp_dd (double x, double xtail);

/* Evaluate order-12 polynomials using pairwise summation and Horner scheme in
   double precision.  */
static inline double
eval_poly_horner_lvl2 (double z, const double *coeff)
{
  double r1, r2, r3, r4, r5, r6, r7, r8;
  double R1, R2, R3, R4;
  double Q1, Q2;
  double z2, z4, z8;
  z2 = z * z;
  r1 = fma (z, coeff[1], coeff[0]);
  r2 = fma (z, coeff[3], coeff[2]);
  z4 = z2 * z2;
  z8 = z4 * z4;
  R1 = fma (z2, r2, r1);
  r3 = fma (z, coeff[5], coeff[4]);
  r4 = fma (z, coeff[7], coeff[6]);
  R2 = fma (z2, r4, r3);
  Q1 = fma (z4, R2, R1);
  r5 = fma (z, coeff[9], coeff[8]);
  r6 = fma (z, coeff[11], coeff[10]);
  R3 = fma (z2, r6, r5);
  r7 = fma (z, coeff[13], coeff[12]);
  r8 = fma (z, coeff[15], coeff[14]);
  R4 = fma (z2, r8, r7);
  Q2 = fma (z4, R4, R3);
  return fma (z8, Q2, Q1);
}

static inline double
eval_exp_mx2 (double x)
{
  return __exp_dd (-(x * x), 0.0);
}

#endif // PL_MATH_ERFCF_H
