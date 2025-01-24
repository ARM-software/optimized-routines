/*
 * Single-precision vector asin(x) function.
 *
 * Copyright (c) 2023-2025, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "v_math.h"
#include "test_sig.h"
#include "test_defs.h"

static const struct data
{
  float32x4_t c0, c2, c4;
  float c1, c3;
  float32x4_t pi_over_2f;
} data = {
  /* Polynomial approximation of  (asin(sqrt(x)) - sqrt(x)) / (x * sqrt(x))  on
     [ 0x1p-24 0x1p-2 ] order = 4 rel error: 0x1.00a23bbp-29 .  */
  .c0 = V4 (0x1.55555ep-3f), .c1 = 0x1.33261ap-4f,
  .c2 = V4 (0x1.70d7dcp-5f), .c3 = 0x1.b059dp-6f,
  .c4 = V4 (0x1.3af7d8p-5f), .pi_over_2f = V4 (0x1.921fb6p+0f),
};

#define AbsMask 0x7fffffff
#define One 0x3f800000
#define Small 0x39800000 /* 2^-12.  */

#if WANT_SIMD_EXCEPT
static float32x4_t VPCS_ATTR NOINLINE
special_case (float32x4_t x, float32x4_t y, uint32x4_t special)
{
  return v_call_f32 (asinf, x, y, special);
}
#endif

/* Single-precision implementation of vector asin(x).


   For |x| <0.5, use order 4 polynomial P such that the final
   approximation is an odd polynomial: asin(x) ~ x + x^3 P(x^2).

    The largest observed error in this region is 0.83 ulps,
      _ZGVnN4v_asinf (0x1.ea00f4p-2) got 0x1.fef15ep-2 want 0x1.fef15cp-2.

    For |x| in [0.5, 1.0], use same approximation with a change of variable

    asin(x) = pi/2 - (y + y * z * P(z)), with  z = (1-x)/2 and y = sqrt(z).

   The largest observed error in this region is 2.41 ulps,
     _ZGVnN4v_asinf (0x1.00203ep-1) got 0x1.0c3a64p-1 want 0x1.0c3a6p-1.  */
float32x4_t VPCS_ATTR NOINLINE V_NAME_F1 (asin) (float32x4_t x)
{
  const struct data *d = ptr_barrier (&data);

  uint32x4_t ix = vreinterpretq_u32_f32 (x);
  uint32x4_t ia = vandq_u32 (ix, v_u32 (AbsMask));

#if WANT_SIMD_EXCEPT
  /* Special values need to be computed with scalar fallbacks so
     that appropriate fp exceptions are raised.  */
  uint32x4_t special
      = vcgtq_u32 (vsubq_u32 (ia, v_u32 (Small)), v_u32 (One - Small));
  if (unlikely (v_any_u32 (special)))
    return special_case (x, x, v_u32 (0xffffffff));
#endif

  float32x4_t ax = vreinterpretq_f32_u32 (ia);
  uint32x4_t a_lt_half = vcaltq_f32 (x, v_f32 (0.5f));

  /* Evaluate polynomial Q(x) = y + y * z * P(z) with
     z = x ^ 2 and y = |x|            , if |x| < 0.5
     z = (1 - |x|) / 2 and y = sqrt(z), if |x| >= 0.5.  */
  float32x4_t z2 = vbslq_f32 (a_lt_half, vmulq_f32 (x, x),
			      vfmsq_n_f32 (v_f32 (0.5f), ax, 0.5f));
  float32x4_t z = vbslq_f32 (a_lt_half, ax, vsqrtq_f32 (z2));

  /* Use a single polynomial approximation P for both intervals.  */

  /* PW Horner 3 evaluation scheme.  */
  float32x4_t z4 = vmulq_f32 (z2, z2);
  float32x4_t c13 = vld1q_f32 (&d->c1);
  float32x4_t p01 = vfmaq_laneq_f32 (d->c0, z2, c13, 0);
  float32x4_t p23 = vfmaq_laneq_f32 (d->c2, z2, c13, 1);
  float32x4_t p = vfmaq_f32 (p23, d->c4, z4);
  p = vfmaq_f32 (p01, p, z4);
  /* Finalize polynomial: z + z * z2 * P(z2).  */
  p = vfmaq_f32 (z, vmulq_f32 (z, z2), p);

  /* asin(|x|) = Q(|x|)         , for |x| < 0.5
	       = pi/2 - 2 Q(|x|), for |x| >= 0.5.  */
  float32x4_t y
      = vbslq_f32 (a_lt_half, p, vfmsq_n_f32 (d->pi_over_2f, p, 2.0f));

  /* Copy sign.  */
  return vbslq_f32 (v_u32 (AbsMask), y, x);
}

HALF_WIDTH_ALIAS_F1 (asin)

TEST_SIG (V, F, 1, asin, -1.0, 1.0)
TEST_ULP (V_NAME_F1 (asin), 1.91)
TEST_DISABLE_FENV_IF_NOT (V_NAME_F1 (asin), WANT_SIMD_EXCEPT)
TEST_INTERVAL (V_NAME_F1 (asin), 0, 0x1p-12, 5000)
TEST_INTERVAL (V_NAME_F1 (asin), 0x1p-12, 0.5, 50000)
TEST_INTERVAL (V_NAME_F1 (asin), 0.5, 1.0, 50000)
TEST_INTERVAL (V_NAME_F1 (asin), 1.0, 0x1p11, 50000)
TEST_INTERVAL (V_NAME_F1 (asin), 0x1p11, inf, 20000)
TEST_INTERVAL (V_NAME_F1 (asin), -0, -inf, 20000)
