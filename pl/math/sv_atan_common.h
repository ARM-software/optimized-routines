/*
 * Double-precision polynomial evaluation function for SVE atan(x) and
 * atan2(y,x).
 *
 * Copyright (c) 2021-2023, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "math_config.h"
#include "sv_math.h"
#include "sv_estrin.h"

#define P(i) sv_f64 (__atan_poly_data.poly[i])

/* Polynomial used in fast SVE atan(x) and atan2(y,x) implementations
   The order 19 polynomial P approximates (atan(sqrt(x))-sqrt(x))/x^(3/2).  */
static inline svfloat64_t
__sv_atan_common (svbool_t pg, svbool_t red, svfloat64_t z, svfloat64_t az,
		  svfloat64_t shift)
{
  /* Use split Estrin scheme for P(z^2) with deg(P)=19. */
  svfloat64_t z2 = svmul_f64_x (pg, z, z);
  svfloat64_t x2 = svmul_f64_x (pg, z2, z2);
  svfloat64_t x4 = svmul_f64_x (pg, x2, x2);
  svfloat64_t x8 = svmul_f64_x (pg, x4, x4);

  svfloat64_t y = FMA (pg, ESTRIN_11_ (pg, z2, x2, x4, x8, P, 8), x8,
		       ESTRIN_7 (pg, z2, x2, x4, P));

  /* Finalize. y = shift + z + z^3 * P(z^2).  */
  svfloat64_t z3 = svmul_f64_x (pg, z2, az);
  y = sv_fma_f64_x (pg, y, z3, az);

  /* Apply shift as indicated by `red` predicate.  */
  y = svadd_f64_m (red, y, shift);

  return y;
}
