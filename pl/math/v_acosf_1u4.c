/*
 * Single-precision vector acos(x) function.
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
#define Pif v_f32 (0x1.921fb6p+1f)
#define Twof v_f32 (2.0f)
#define Onef v_f32 (1.0f)
#define MOnef v_f32 (-1.0f)
#define Half (0x3f000000)
#define One (0x3f800000)
#define Small (0x32800000) /* 2^-26.  */

#define P(i) v_f32 (__asinf_poly[i])

#if WANT_SIMD_EXCEPT
static NOINLINE v_f32_t
special_case (v_f32_t x, v_f32_t y, v_u32_t special)
{
  return v_call_f32 (acosf, x, y, special);
}
#endif

/* Single-precision implementation of vector acos(x).

   For |x| < Small, approximate acos(x) by pi/2 - x. Small = 2^-26 for correct
   rounding.
   If WANT_SIMD_EXCEPT = 0, Small = 0 and we proceed with the following
   approximation.

   For |x| in [Small, 0.5], use order 4 polynomial P such that the final
   approximation of asin is an odd polynomial:

     acos(x) ~ pi/2 - (x + x^3 P(x^2)).

    The largest observed error in this region is 1.26 ulps,
      __v_acosf(0x1.843bfcp-2) got 0x1.2e934cp+0 want 0x1.2e934ap+0.

    For |x| in [0.5, 1.0], use same approximation with a change of variable

      acos(x) = y + y * z * P(z), with  z = (1-x)/2 and y = sqrt(z).

    This approach is described in more details in scalar acosf.

   The largest observed error in this region is 1.32 ulps,
   __v_acosf(0x1.15ba56p-1) got 0x1.feb33p-1
			   want 0x1.feb32ep-1.  */
VPCS_ATTR v_f32_t V_NAME (acosf) (v_f32_t x)
{
  v_u32_t ix = v_as_u32_f32 (x);
  v_u32_t ia = ix & AbsMask;

#if WANT_SIMD_EXCEPT
  /* A single comparison for One, Small and QNaN.  */
  v_u32_t special = v_cond_u32 (ia - Small > One - Small);
  if (unlikely (v_any_u32 (special)))
    return special_case (x, x, v_u32 (0xffffffff));
#else
  /* Fixing sign of NaN when x < -1.0.  */
  ix = v_sel_u32 (v_cond_u32 (x < MOnef), v_u32 (0), ix);
#endif

  v_f32_t ax = v_as_f32_u32 (ia);
  v_u32_t a_le_half = v_cond_u32 (ia <= Half);

  /* Evaluate polynomial Q(x) = z + z * z2 * P(z2) with
     z2 = x ^ 2         and z = |x|     , if |x| < 0.5
     z2 = (1 - |x|) / 2 and z = sqrt(z2), if |x| >= 0.5.  */
  v_f32_t z2 = v_sel_f32 (a_le_half, x * x, v_fma_f32 (-Halff, ax, Halff));
  v_f32_t z = v_sel_f32 (a_le_half, ax, v_sqrt_f32 (z2));

  /* Use a single polynomial approximation P for both intervals.  */
  v_f32_t p = HORNER_4 (z2, P);
  /* Finalize polynomial: z + z * z2 * P(z2).  */
  p = v_fma_f32 (z * z2, p, z);

  /* acos(|x|) = pi/2 - sign(x) * Q(|x|), for  |x| < 0.5
	       = 2 Q(|x|)               , for  0.5 < x < 1.0
	       = pi - 2 Q(|x|)          , for -1.0 < x < -0.5.  */
  v_f32_t y = v_as_f32_u32 (v_bsl_u32 (v_u32 (AbsMask), v_as_u32_f32 (p), ix));

  v_u32_t sign = v_cond_u32 (x < 0);
  v_f32_t off = v_sel_f32 (sign, Pif, v_f32 (0.0f));
  v_f32_t mul = v_sel_f32 (a_le_half, -Onef, Twof);
  v_f32_t add = v_sel_f32 (a_le_half, PiOver2f, off);

  return v_fma_f32 (mul, y, add);
}
PL_ALIAS (V_NAME (acosf), _ZGVnN4v_acosf)

PL_SIG (V, F, 1, acos, -1.0, 1.0)
PL_TEST_ULP (V_NAME (acosf), 0.82)
PL_TEST_EXPECT_FENV (V_NAME (acosf), WANT_SIMD_EXCEPT)
PL_TEST_INTERVAL (V_NAME (acosf), 0, Small, 5000)
PL_TEST_INTERVAL (V_NAME (acosf), Small, 0.5, 50000)
PL_TEST_INTERVAL (V_NAME (acosf), 0.5, 1.0, 50000)
PL_TEST_INTERVAL (V_NAME (acosf), 1.0, 0x1p11, 50000)
PL_TEST_INTERVAL (V_NAME (acosf), 0x1p11, inf, 20000)
PL_TEST_INTERVAL (V_NAME (acosf), -0, -inf, 20000)
