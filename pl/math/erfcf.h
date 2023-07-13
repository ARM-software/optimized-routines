/*
 * Shared functions for scalar and vector single-precision erfc(x) functions.
 *
 * Copyright (c) 2021-2023, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#ifndef PL_MATH_ERFCF_H
#define PL_MATH_ERFCF_H

#include "math_config.h"
#include "poly_scalar_f64.h"

/* Accurate exponential from optimized-routines.  */
double
__exp_dd (double x, double xtail);

static inline double
eval_poly (double z, const double *coeff)
{
  double z2 = z * z;
  double z4 = z2 * z2;
  double z8 = z4 * z4;
  return estrin_15_f64 (z, z2, z4, z8, coeff);
}

static inline double
eval_exp_mx2 (double x)
{
  return __exp_dd (-(x * x), 0.0);
}

#endif // PL_MATH_ERFCF_H
