/*
 * Single-precision polynomial evaluation function for SVE atan(x) and
 * atan2(y,x).
 *
 * Copyright (c) 2021-2023, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#ifndef PL_MATH_SV_ATANF_COMMON_H
#define PL_MATH_SV_ATANF_COMMON_H

#include "math_config.h"
#include "sv_math.h"
#include "sv_estrinf.h"

#define P(i) sv_f32 (__atanf_poly_data.poly[i])

/* Polynomial used in fast SVE atanf(x) and atan2f(y,x) implementations
   The order 7 polynomial P approximates (f(sqrt(x))-sqrt(x))/x^(3/2).  */
static inline svfloat32_t
__sv_atanf_common (svbool_t pg, svbool_t red, svfloat32_t z, svfloat32_t az,
		   svfloat32_t shift)
{
  /* Use split Estrin scheme for P(z^2) with deg(P)=7. */

  /* First compute square powers of z.  */
  svfloat32_t z2 = svmul_f32_x (pg, z, z);
  svfloat32_t z4 = svmul_f32_x (pg, z2, z2);
  svfloat32_t z8 = svmul_f32_x (pg, z4, z4);

  /* Then assemble polynomial.  */
  svfloat32_t y = ESTRIN_7 (pg, z2, z4, z8, P);

  /* Finalize. y = shift + z + z^3 * P(z^2).  */
  svfloat32_t z3 = svmul_f32_x (pg, z2, az);
  y = svmla_f32_x (pg, az, z3, y);

  /* Apply shift as indicated by 'red' predicate.  */
  y = svadd_f32_m (red, y, shift);

  return y;
}

#endif // PL_MATH_SV_ATANF_COMMON_H
