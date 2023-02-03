/*
 * Single-precision vector asin(x) function.
 *
 * Copyright (c) 2023, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "v_math.h"
#include "hornerf.h"
#include "pl_sig.h"
#include "pl_test.h"

#define AbsMask (0x7fffffff)
#define Halff v_f32 (0.5f)
#define PiOver2f v_f32 (0x1.921fb6p+0f)
#define Twof v_f32 (2.0f)
#define MOnef v_f32 (-1.0f)
#define Half (0x3f000000)
#define One (0x3f800000)
#define Small (0x39800000) /* 2^-12.  */

#define P(i) v_f32 (__asinf_poly[i])

#if WANT_SIMD_EXCEPT
static NOINLINE v_f32_t
specialcase (v_f32_t x, v_f32_t y, v_u32_t special)
{
  return v_call_f32 (asinf, x, y, special);
}
#endif

/* Single-precision implementation of vector asin(x).

   For |x| < Small, approximate asin(x) by x. Small = 2^-12 for correct
   rounding. If WANT_SIMD_EXCEPT = 0, Small = 0 and we proceed with the
   following approximation.

   For |x| in [Small, 0.5], use order 4 polynomial P such that the final
   approximation is an odd polynomial: asin(x) ~ x + x^3 P(x^2).

    The largest observed error in this region is 0.83 ulps,
      __v_asinf(0x1.ea00f4p-2) got 0x1.fef15ep-2 want 0x1.fef15cp-2.

    For |x| in [0.5, 1.0], use same approximation with a change of variable

    asin(x) = pi/2 - (y + y * z * P(z)), with  z = (1-x)/2 and y = sqrt(z).

    This approach is described in more details in the scalar asinf.

   The largest observed error in this region is 2.41 ulps,
     __v_asinf(0x1.00203ep-1) got 0x1.0c3a64p-1 want 0x1.0c3a6p-1.  */
VPCS_ATTR v_f32_t V_NAME (asinf) (v_f32_t x)
{
  v_u32_t ix = v_as_u32_f32 (x);
  v_u32_t ia = ix & AbsMask;

#if WANT_SIMD_EXCEPT
  /* Special values need to be computed with scalar fallbacks so
     that appropriate fp exceptions are raised.  */
  v_u32_t special = v_cond_u32 (ia - Small > One - Small);
  if (unlikely (v_any_u32 (special)))
    return specialcase (x, x, v_u32 (0xffffffff));
#else
  /* Fixing sign of NaN when x < -1.0.  */
  ix = v_sel_u32 (v_cond_u32 (x < MOnef), v_u32 (0), ix);
#endif

  v_f32_t ax = v_as_f32_u32 (ia);
  v_u32_t a_lt_half = v_cond_u32 (ia < Half);

  /* Evaluate polynomial Q(x) = y + y * z * P(z) with
     z = x ^ 2 and y = |x|            , if |x| < 0.5
     z = (1 - |x|) / 2 and y = sqrt(z), if |x| >= 0.5.  */
  v_f32_t z2 = v_sel_f32 (a_lt_half, x * x, v_fma_f32 (-Halff, ax, Halff));
  v_f32_t z = v_sel_f32 (a_lt_half, ax, v_sqrt_f32 (z2));

  /* Use a single polynomial approximation P for both intervals.  */
  v_f32_t p = HORNER_4 (z2, P);
  /* Finalize polynomial: z + z * z2 * P(z2).  */
  p = v_fma_f32 (z * z2, p, z);

  /* asin(|x|) = Q(|x|)         , for |x| < 0.5
	       = pi/2 - 2 Q(|x|), for |x| >= 0.5.  */
  v_f32_t y = v_sel_f32 (a_lt_half, p, v_fma_f32 (-Twof, p, PiOver2f));

  /* Copy sign.  */
  return v_as_f32_u32 (v_bsl_u32 (v_u32 (AbsMask), v_as_u32_f32 (y), ix));
}
PL_ALIAS (V_NAME (asinf), _ZGVnN4v_asinf)

PL_SIG (V, F, 1, asin, -1.0, 1.0)
PL_TEST_ULP (V_NAME (asinf), 1.91)
PL_TEST_EXPECT_FENV (V_NAME (asinf), WANT_SIMD_EXCEPT)
PL_TEST_INTERVAL (V_NAME (asinf), 0, Small, 5000)
PL_TEST_INTERVAL (V_NAME (asinf), Small, 0.5, 50000)
PL_TEST_INTERVAL (V_NAME (asinf), 0.5, 1.0, 50000)
PL_TEST_INTERVAL (V_NAME (asinf), 1.0, 0x1p11, 50000)
PL_TEST_INTERVAL (V_NAME (asinf), 0x1p11, inf, 20000)
PL_TEST_INTERVAL (V_NAME (asinf), -0, -inf, 20000)
