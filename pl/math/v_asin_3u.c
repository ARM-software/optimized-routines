/*
 * Double-precision vector asin(x) function.
 *
 * Copyright (c) 2023, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "v_math.h"
#include "estrin.h"
#include "pl_sig.h"
#include "pl_test.h"

#define AllMask v_u64 (0xffffffffffffffff)
#define AbsMask (0x7fffffffffffffff)
#define Half v_f64 (0.5)
#define PiOver2 v_f64 (0x1.921fb54442d18p+0)
#define Two v_f64 (2.0)
#define MOne v_f64 (-1.0)
#define Halfu (0x3fe0000000000000)
#define One (0x3ff0000000000000)
#define Small (0x3e50000000000000) /* 2^-12.  */

#define P(i) v_f64 (__asin_poly[i])

#if WANT_SIMD_EXCEPT
static NOINLINE float64x2_t
special_case (float64x2_t x, float64x2_t y, uint64x2_t special)
{
  return v_call_f64 (asin, x, y, special);
}
#endif

/* Double-precision implementation of vector asin(x).

   For |x| < Small, approximate asin(x) by x. Small = 2^-12 for correct
   rounding. If WANT_SIMD_EXCEPT = 0, Small = 0 and we proceed with the
   following approximation.

   For |x| in [Small, 0.5], use an order 11 polynomial P such that the final
   approximation is an odd polynomial: asin(x) ~ x + x^3 P(x^2).

   The largest observed error in this region is 1.01 ulps,
   __v_asin(0x1.da9735b5a9277p-2) got 0x1.ed78525a927efp-2
				 want 0x1.ed78525a927eep-2.

   For |x| in [0.5, 1.0], use same approximation with a change of variable

     asin(x) = pi/2 - (y + y * z * P(z)), with  z = (1-x)/2 and y = sqrt(z).

   This approach is described in more details in the scalar asin.

   The largest observed error in this region is 2.69 ulps,
   __v_asin(0x1.044ac9819f573p-1) got 0x1.110d7e85fdd5p-1
				 want 0x1.110d7e85fdd53p-1.  */
VPCS_ATTR float64x2_t V_NAME_D1 (asin) (float64x2_t x)
{
  uint64x2_t ix = v_as_u64_f64 (x);
  uint64x2_t ia = ix & AbsMask;

#if WANT_SIMD_EXCEPT
  /* Special values need to be computed with scalar fallbacks so
     that appropriate exceptions are raised.  */
  uint64x2_t special = ia - Small > One - Small;
  if (unlikely (v_any_u64 (special)))
    return special_case (x, x, AllMask);
#else
  /* Fixing sign of NaN when x < -1.0.  */
  ix = vbslq_u64 (x < MOne, v_u64 (0), ix);
#endif

  float64x2_t ax = v_as_f64_u64 (ia);
  uint64x2_t a_lt_half = ia < Halfu;

  /* Evaluate polynomial Q(x) = y + y * z * P(z) with
     z = x ^ 2 and y = |x|            , if |x| < 0.5
     z = (1 - |x|) / 2 and y = sqrt(z), if |x| >= 0.5.  */
  float64x2_t z2 = vbslq_f64 (a_lt_half, x * x, v_fma_f64 (-Half, ax, Half));
  float64x2_t z = vbslq_f64 (a_lt_half, ax, vsqrtq_f64 (z2));

  /* Use a single polynomial approximation P for both intervals.  */
  float64x2_t z4 = z2 * z2;
  float64x2_t z8 = z4 * z4;
  float64x2_t z16 = z8 * z8;
  float64x2_t p = ESTRIN_11 (z2, z4, z8, z16, P);

  /* Finalize polynomial: z + z * z2 * P(z2).  */
  p = v_fma_f64 (z * z2, p, z);

  /* asin(|x|) = Q(|x|)         , for |x| < 0.5
	       = pi/2 - 2 Q(|x|), for |x| >= 0.5.  */
  float64x2_t y = vbslq_f64 (a_lt_half, p, v_fma_f64 (-Two, p, PiOver2));

  /* Copy sign.  */
  return v_as_f64_u64 (vbslq_u64 (v_u64 (AbsMask), v_as_u64_f64 (y), ix));
}

PL_SIG (V, D, 1, asin, -1.0, 1.0)
PL_TEST_ULP (V_NAME_D1 (asin), 2.19)
PL_TEST_EXPECT_FENV (V_NAME_D1 (asin), WANT_SIMD_EXCEPT)
PL_TEST_INTERVAL (V_NAME_D1 (asin), 0, Small, 5000)
PL_TEST_INTERVAL (V_NAME_D1 (asin), Small, 0.5, 50000)
PL_TEST_INTERVAL (V_NAME_D1 (asin), 0.5, 1.0, 50000)
PL_TEST_INTERVAL (V_NAME_D1 (asin), 1.0, 0x1p11, 50000)
PL_TEST_INTERVAL (V_NAME_D1 (asin), 0x1p11, inf, 20000)
PL_TEST_INTERVAL (V_NAME_D1 (asin), -0, -inf, 20000)
