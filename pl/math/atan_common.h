/*
 * Double-precision polynomial evaluation function for scalar
 * atan(x) and atan2(y,x).
 *
 * Copyright (c) 2021-2023, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "math_config.h"
#include "estrin.h"

#define P(i) __atan_poly_data.poly[i]

/* Polynomial used in fast atan(x) and atan2(y,x) implementations
   The order 19 polynomial P approximates (atan(sqrt(x))-sqrt(x))/x^(3/2).  */
static inline double
eval_poly (double z, double az, double shift)
{
  /* Use split Estrin scheme for P(z^2) with deg(P)=19. Use split instead of
     full scheme to avoid underflow in x^16.  */
  double z2 = z * z;
  double x2 = z2 * z2;
  double x4 = x2 * x2;
  double x8 = x4 * x4;
  double y
      = fma (ESTRIN_11_ (z2, x2, x4, x8, P, 8), x8, ESTRIN_7 (z2, x2, x4, P));

  /* Finalize. y = shift + z + z^3 * P(z^2).  */
  y = fma (y, z2 * az, az);
  y = y + shift;

  return y;
}

#undef P
