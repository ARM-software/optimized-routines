/*
 * Double-precision polynomial evaluation function for scalar and vector
 * log1p(x)
 *
 * Copyright (c) 2022, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#ifndef PL_MATH_LOG1P_COMMON_H
#define PL_MATH_LOG1P_COMMON_H

#include "math_config.h"

#if V_SUPPORTED

#include "v_math.h"

#define DBL_T v_f64_t
#define FMA v_fma_f64
#define C(i) v_f64 (__log1p_data.coeffs[i])

#else

#define DBL_T double
#define FMA fma
#define C(i) __log1p_data.coeffs[i]

#endif

static inline DBL_T
eval_poly (DBL_T f)
{
  /* Evaluate polynomial using Estrin's method.  */
  DBL_T p_01 = FMA (f, C (1), C (0));
  DBL_T p_23 = FMA (f, C (3), C (2));
  DBL_T p_45 = FMA (f, C (5), C (4));
  DBL_T p_67 = FMA (f, C (7), C (6));
  DBL_T p_89 = FMA (f, C (9), C (8));
  DBL_T p_ab = FMA (f, C (11), C (10));
  DBL_T p_cd = FMA (f, C (13), C (12));
  DBL_T p_ef = FMA (f, C (15), C (14));
  DBL_T p_gh = FMA (f, C (17), C (16));

  DBL_T f2 = f * f;
  DBL_T p_03 = FMA (f2, p_23, p_01);
  DBL_T p_47 = FMA (f2, p_67, p_45);
  DBL_T p_8b = FMA (f2, p_ab, p_89);
  DBL_T p_cf = FMA (f2, p_ef, p_cd);
  DBL_T p_gi = FMA (f2, C (18), p_gh);

  DBL_T f4 = f2 * f2;
  DBL_T p_07 = FMA (f4, p_47, p_03);
  DBL_T p_8f = FMA (f4, p_cf, p_8b);

  DBL_T f8 = f4 * f4;
  DBL_T p_0f = FMA (f8, p_8f, p_07);

  return FMA (f8 * f8, p_gi, p_0f);
}

#endif // PL_MATH_LOG1P_COMMON_H
