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
static NOINLINE float32x4_t
specialcase (float32x4_t x, float32x4_t y, uint32x4_t special)
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
VPCS_ATTR float32x4_t V_NAME_F1 (asin) (float32x4_t x)
{
  uint32x4_t ix = v_as_u32_f32 (x);
  uint32x4_t ia = ix & AbsMask;

#if WANT_SIMD_EXCEPT
  /* Special values need to be computed with scalar fallbacks so
     that appropriate fp exceptions are raised.  */
  uint32x4_t special = ia - Small > One - Small;
  if (unlikely (v_any_u32 (special)))
    return specialcase (x, x, v_u32 (0xffffffff));
#else
  /* Fixing sign of NaN when x < -1.0.  */
  ix = vbslq_u32 (x < MOnef, v_u32 (0), ix);
#endif

  float32x4_t ax = v_as_f32_u32 (ia);
  uint32x4_t a_lt_half = ia < Half;

  /* Evaluate polynomial Q(x) = y + y * z * P(z) with
     z = x ^ 2 and y = |x|            , if |x| < 0.5
     z = (1 - |x|) / 2 and y = sqrt(z), if |x| >= 0.5.  */
  float32x4_t z2 = vbslq_f32 (a_lt_half, x * x, vfmaq_f32 (Halff, -Halff, ax));
  float32x4_t z = vbslq_f32 (a_lt_half, ax, vsqrtq_f32 (z2));

  /* Use a single polynomial approximation P for both intervals.  */
  float32x4_t p = HORNER_4 (z2, P);
  /* Finalize polynomial: z + z * z2 * P(z2).  */
  p = vfmaq_f32 (z, z * z2, p);

  /* asin(|x|) = Q(|x|)         , for |x| < 0.5
	       = pi/2 - 2 Q(|x|), for |x| >= 0.5.  */
  float32x4_t y = vbslq_f32 (a_lt_half, p, vfmaq_f32 (PiOver2f, -Twof, p));

  /* Copy sign.  */
  return v_as_f32_u32 (vbslq_u32 (v_u32 (AbsMask), v_as_u32_f32 (y), ix));
}

PL_SIG (V, F, 1, asin, -1.0, 1.0)
PL_TEST_ULP (V_NAME_F1 (asin), 1.91)
PL_TEST_EXPECT_FENV (V_NAME_F1 (asin), WANT_SIMD_EXCEPT)
PL_TEST_INTERVAL (V_NAME_F1 (asin), 0, Small, 5000)
PL_TEST_INTERVAL (V_NAME_F1 (asin), Small, 0.5, 50000)
PL_TEST_INTERVAL (V_NAME_F1 (asin), 0.5, 1.0, 50000)
PL_TEST_INTERVAL (V_NAME_F1 (asin), 1.0, 0x1p11, 50000)
PL_TEST_INTERVAL (V_NAME_F1 (asin), 0x1p11, inf, 20000)
PL_TEST_INTERVAL (V_NAME_F1 (asin), -0, -inf, 20000)
