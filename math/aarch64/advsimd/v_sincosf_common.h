/*
 * Core approximation for single-precision vector sincos
 *
 * Copyright (c) 2023-2026, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "v_math.h"
#include "v_trigf_fallback.h"

const static struct v_sincosf_data
{
  float inv_pio2, pio2_1, pio2_2, pio2_3;
  float s2, c2, c0, null;
  float32x4_t s0, s1, c1, shift, range_val;
} v_sincosf_data = {
  .s0 = V4 (-0x1.555546p-3f),
  .s1 = V4 (0x1.11076p-7f),
  .s2 = -0x1.994eb4p-13f,
  .c0 = 0x1.55554ap-5f,
  .c1 = V4 (-0x1.6c0c1ap-10f),
  .c2 = 0x1.99e0eep-16f,

  .inv_pio2 = 0x1.45f306p-1f,
  .pio2_1 = 0x1.921fb6p+0f,
  .pio2_2 = -0x1.777a5cp-25f,
  .pio2_3 = -0x1.ee59dap-50f,

  .shift = V4 (0x1.8p23),
  .range_val = V4 (0x1p20),
};

static inline uint32x4_t
check_ge_rangeval (float32x4_t x, const struct v_sincosf_data *d)
{
  return vcagtq_f32 (x, d->range_val);
}

/* Single-precision vector function allowing calculation of both sin and cos in
   one function call, using shared argument reduction and separate low-order
   polynomials.
   Worst-case error for sin is 1.67 ULP:
   v_sincosf_sin(0x1.c704c4p+19) got 0x1.fff698p-5 want 0x1.fff69cp-5
   Worst-case error for cos is 1.81 ULP:
   v_sincosf_cos(0x1.e506fp+19) got -0x1.ffec6ep-6 want -0x1.ffec72p-6.  */
static inline float32x4x2_t
v_sincosf_inline (float32x4_t x, const struct v_sincosf_data *d)
{
  float32x4_t pio2_vals = vld1q_f32 (&d->inv_pio2);
  /* n = rint ( x / (pi/2) ).  */
  float32x4_t shift = d->shift;
  float32x4_t q = vfmaq_laneq_f32 (shift, x, pio2_vals, 0);
  q = vsubq_f32 (q, shift);
  int32x4_t n = vcvtq_s32_f32 (q);

  /* Reduce x such that r is in [ -pi/4, pi/4 ].  */
  float32x4_t r = x;
  r = vfmsq_laneq_f32 (r, q, pio2_vals, 1);
  r = vfmsq_laneq_f32 (r, q, pio2_vals, 2);
  r = vfmsq_laneq_f32 (r, q, pio2_vals, 3);

  float32x4_t coeffs = vld1q_f32 (&d->s2);

  /* Approximate sin(r) ~= r + r^3 * poly_sin(r^2).  */
  float32x4_t r2 = vmulq_f32 (r, r);
  float32x4_t r3 = vmulq_f32 (r, r2);
  float32x4_t s;
  s = vfmaq_laneq_f32 (d->s1, r2, coeffs, 0);
  s = vfmaq_f32 (d->s0, r2, s);
  s = vfmaq_f32 (r, r3, s);

  /* Approximate cos(r) ~= 1 - (r^2)/2 + r^4 * poly_cos(r^2).  */
  float32x4_t r4 = vmulq_f32 (r2, r2);
  float32x4_t p = vfmaq_laneq_f32 (d->c1, r2, coeffs, 1);
  float32x4_t c;
  c = vfmaq_laneq_f32 (v_f32 (-0.5), r2, coeffs, 2);
  c = vfmaq_f32 (c, r4, p);
  c = vfmaq_f32 (v_f32 (1), c, r2);

  /* If odd quadrant, swap cos and sin.  */
  uint32x4_t swap = vtstq_u32 (vreinterpretq_u32_s32 (n), v_u32 (1));
  float32x4_t ss = vbslq_f32 (swap, c, s);
  float32x4_t cc = vbslq_f32 (swap, s, c);

  /* Fix signs according to quadrant.
     ss = asfloat(asuint(ss) ^ ((n       & 2) << 30))
     cc = asfloat(asuint(cc) & (((n + 1) & 2) << 30)).  */
  uint32x4_t sin_sign
      = vshlq_n_u32 (vandq_u32 (vreinterpretq_u32_s32 (n), v_u32 (2)), 30);
  uint32x4_t cos_sign = vshlq_n_u32 (
      vandq_u32 (vreinterpretq_u32_s32 (vaddq_s32 (n, v_s32 (1))), v_u32 (2)),
      30);
  ss = vreinterpretq_f32_u32 (
      veorq_u32 (vreinterpretq_u32_f32 (ss), sin_sign));
  cc = vreinterpretq_f32_u32 (
      veorq_u32 (vreinterpretq_u32_f32 (cc), cos_sign));

  return (float32x4x2_t){ ss, cc };
}

/* Vectorsied fallback for sincosf and cexpif for large input arguments.
   Worst-case error for sin is 1.42 + 0.5 ULP:
   sv_sincosf_sin (0x1.dbe63cp+36) got -0x1.d76b4p-2 want -0x1.d76b44p-2.
   Worst-case error for cos is 1.43 + 0.5 ULP:
   sv_sincosf_cos (0x1.a2785ap+108) got -0x1.dc17fp-2 want -0x1.dc17f4p-2.  */
static inline float32x4x2_t VPCS_ATTR
sincos_fallback (float32x4_t x)
{
  struct reduction_result_t r = large_range_reduction (x);
  float32x4x2_t lookup = sin_cos_lookup (r.octant);
  float32x4x2_t eval_fast = sincos_eval (r.remainder);

  /* Construct sin(x) and cos (x)from k and r, using angle addition formula,
     with approximations of sin(r) and cos(r) - 1 to reduce rounding errors.
     sin(x) = sin(k + r)
	    = cos(k)*sin(r) + sin(k)*cos(r)
	    = cos(k)*sin(r) + sin(k)*cosm1(r) + sin(k).
     cos(x) = cos(k + r)
	     = cos(k)*cos(r) - sin(k)*sin(r)
	     = cos(k)*cosm1(r) - sin(k)*sin(r) + cos(k).  */

  float32x4_t sin_k = lookup.val[0];
  float32x4_t cos_k = lookup.val[1];
  float32x4_t sin_r = eval_fast.val[0];
  float32x4_t cosm1_r = eval_fast.val[1];

  float32x4_t sin_k_cosm1_r = vmulq_f32 (sin_k, cosm1_r);
  float32x4_t sin = vfmaq_f32 (sin_k_cosm1_r, cos_k, sin_r);

  float32x4_t cos_k_cosm1_r = vmulq_f32 (cos_k, cosm1_r);
  float32x4_t cos = vfmsq_f32 (cos_k_cosm1_r, sin_k, sin_r);
  float32x4x2_t result;
  result.val[0] = vaddq_f32 (sin, sin_k);
  result.val[1] = vaddq_f32 (cos, cos_k);
  return result;
}
