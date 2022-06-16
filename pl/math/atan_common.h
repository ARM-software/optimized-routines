/*
 * Double-precision polynomial evaluation function for scalar and vector atan(x)
 * and atan2(y,x).
 *
 * Copyright (c) 2021-2022, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "math_config.h"

#if V_SUPPORTED

#include "v_math.h"

#define DBL_T v_f64_t
#define FMA v_fma_f64
#define P(i) v_f64 (__atan_poly_data.poly[i])

#else

#define DBL_T double
#define FMA fma
#define P(i) __atan_poly_data.poly[i]

#endif

/* Polynomial used in fast atan(x) and atan2(y,x) implementations
   The order 19 polynomial P approximates (atan(sqrt(x))-sqrt(x))/x^(3/2).  */
static inline DBL_T
eval_poly (DBL_T z, DBL_T az, DBL_T shift)
{
  /* Use full Estrin scheme for P(z^2) with deg(P)=19.  */
  DBL_T z2 = z * z;
  /* Level 1.  */
  DBL_T P_1_0 = FMA (P (1), z2, P (0));
  DBL_T P_3_2 = FMA (P (3), z2, P (2));
  DBL_T P_5_4 = FMA (P (5), z2, P (4));
  DBL_T P_7_6 = FMA (P (7), z2, P (6));
  DBL_T P_9_8 = FMA (P (9), z2, P (8));
  DBL_T P_11_10 = FMA (P (11), z2, P (10));
  DBL_T P_13_12 = FMA (P (13), z2, P (12));
  DBL_T P_15_14 = FMA (P (15), z2, P (14));
  DBL_T P_17_16 = FMA (P (17), z2, P (16));
  DBL_T P_19_18 = FMA (P (19), z2, P (18));

  /* Level 2.  */
  DBL_T x2 = z2 * z2;
  DBL_T P_3_0 = FMA (P_3_2, x2, P_1_0);
  DBL_T P_7_4 = FMA (P_7_6, x2, P_5_4);
  DBL_T P_11_8 = FMA (P_11_10, x2, P_9_8);
  DBL_T P_15_12 = FMA (P_15_14, x2, P_13_12);
  DBL_T P_19_16 = FMA (P_19_18, x2, P_17_16);

  /* Level 3.  */
  DBL_T x4 = x2 * x2;
  DBL_T P_7_0 = FMA (P_7_4, x4, P_3_0);
  DBL_T P_15_8 = FMA (P_15_12, x4, P_11_8);

  /* Level 4.  */
  DBL_T x8 = x4 * x4;
  DBL_T y = FMA (P_19_16, x8, P_15_8);
  y = FMA (y, x8, P_7_0);

  /* Finalize. y = shift + z + z^3 * P(z^2).  */
  y = FMA (y, z2 * az, az);
  y = y + shift;

  return y;
}

#undef DBL_T
#undef FMA
#undef P
