/*
 * Single-precision vector 10^x - 1 function.
 *
 * Copyright (c) 2025, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "v_math.h"
#include "test_sig.h"
#include "test_defs.h"

#define ScaleBound 192.0f
/* rint (log2 (2^127 / (1 + sqrt (2)))).  */
#define SpecialBound 126.0f

static const struct data
{
  float log10_2_high, log10_2_low;
  float log10_lo, c2, c4, c6;
  float32x4_t log10_hi, c1, c3, c5, c7, c8;
  float32x4_t inv_log10_2, special_bound;
  uint32x4_t exponent_bias, special_offset, special_bias;
  float32x4_t scale_thresh;
} data = {
  /* Coefficients generated using Remez algorithm with minimisation of relative
     error.  */
  .log10_hi = V4 (0x1.26bb1b8000000p+1),
  .log10_lo = 0x1.daaa8b0000000p-26,
  .c1 = V4 (0x1.53524ep1),
  .c2 = 0x1.046fc8p1,
  .c3 = V4 (0x1.2bd376p0),
  .c4 = 0x1.156f8p-1,
  .c5 = V4 (0x1.b28c0ep-3),
  .c6 = -0x1.05e38ep-4,
  .c7 = V4 (-0x1.c79f4ap-4),
  .c8 = V4 (0x1.2d6f34p1),
  .inv_log10_2 = V4 (0x1.a934fp+1),
  .log10_2_high = 0x1.344136p-2,
  .log10_2_low = 0x1.ec10cp-27,
  .special_bound = V4 (SpecialBound),
  .exponent_bias = V4 (0x3f800000),
  .special_offset = V4 (0x82000000),
  .special_bias = V4 (0x7f000000),
  .scale_thresh = V4 (ScaleBound)
};

static float32x4_t VPCS_ATTR NOINLINE
special_case (float32x4_t poly, float32x4_t n, uint32x4_t e, uint32x4_t cmp1,
	      float32x4_t scale, const struct data *d)
{
  /* 2^n may overflow, break it up into s1*s2.  */
  uint32x4_t b = vandq_u32 (vclezq_f32 (n), d->special_offset);
  float32x4_t s1 = vreinterpretq_f32_u32 (vaddq_u32 (b, d->special_bias));
  float32x4_t s2 = vreinterpretq_f32_u32 (vsubq_u32 (e, b));
  uint32x4_t cmp2 = vcagtq_f32 (n, d->scale_thresh);
  float32x4_t r2 = vmulq_f32 (s1, s1);
  float32x4_t r1 = vmulq_f32 (vfmaq_f32 (s2, poly, s2), s1);
  /* Similar to r1 but avoids double rounding in the subnormal range.  */
  float32x4_t r0 = vfmaq_f32 (scale, poly, scale);
  float32x4_t r = vbslq_f32 (cmp1, r1, r0);
  return vsubq_f32 (vbslq_f32 (cmp2, r2, r), v_f32 (1.0));
}

/* Fast vector implementation of single-precision exp10.
   Algorithm is accurate to 1.70 + 0.5 ULP.
   _ZGVnN4v_exp10m1f(0x1.36f94cp-3) got 0x1.ac96acp-2
				   want 0x1.ac96bp-2.  */
float32x4_t VPCS_ATTR NOINLINE V_NAME_F1 (exp10m1) (float32x4_t x)
{
  const struct data *d = ptr_barrier (&data);

  /* exp10(x) = 2^n * 10^r = 2^n * (1 + poly (r)),
     with poly(r) in [1/sqrt(2), sqrt(2)] and
     x = r + n * log10 (2), with r in [-log10(2)/2, log10(2)/2].  */
  float32x4_t log10_2 = vld1q_f32 (&d->log10_2_high);
  float32x4_t n = vrndaq_f32 (vmulq_f32 (x, d->inv_log10_2));
  float32x4_t r = vfmsq_laneq_f32 (x, n, log10_2, 0);
  r = vfmaq_laneq_f32 (r, n, log10_2, 1);
  uint32x4_t e = vshlq_n_u32 (vreinterpretq_u32_s32 (vcvtaq_s32_f32 (n)), 23);

  float32x4_t scale = vreinterpretq_f32_u32 (vaddq_u32 (e, d->exponent_bias));
  uint32x4_t cmp = vcagtq_f32 (n, d->special_bound);

  /* Pairwise Horner scheme.  */
  float32x4_t log10lo_c246 = vld1q_f32 (&d->log10_lo);
  float32x4_t r2 = vmulq_f32 (r, r);
  float32x4_t p78 = vfmaq_f32 (d->c7, r, d->c8);
  float32x4_t p56 = vfmaq_laneq_f32 (d->c5, r, log10lo_c246, 3);
  float32x4_t p34 = vfmaq_laneq_f32 (d->c3, r, log10lo_c246, 2);
  float32x4_t p12 = vfmaq_laneq_f32 (d->c1, r, log10lo_c246, 1);
  float32x4_t p58 = vfmaq_f32 (p56, r2, p78);
  float32x4_t p36 = vfmaq_f32 (p34, r2, p58);
  float32x4_t p16 = vfmaq_f32 (p12, r2, p36);

  float32x4_t poly
      = vfmaq_laneq_f32 (vmulq_f32 (d->log10_hi, r), r, log10lo_c246, 0);
  poly = vfmaq_f32 (poly, p16, r2);

  float32x4_t ret = vfmaq_f32 (vsubq_f32 (scale, v_f32 (1.0)), poly, scale);
  if (unlikely (v_any_u32 (cmp)))
    return vbslq_f32 (cmp, special_case (poly, n, e, cmp, scale, d), ret);

  return ret;
}

HALF_WIDTH_ALIAS_F1 (exp10m1)

#if WANT_C23_TESTS
TEST_ULP (V_NAME_F1 (exp10m1), 1.70)
TEST_DISABLE_FENV (V_NAME_F1 (exp10m1))
TEST_INTERVAL (V_NAME_F1 (exp10m1), 0, 0xffff0000, 10000)
TEST_SYM_INTERVAL (V_NAME_F1 (exp10m1), 0x1p-14, 0x1p-1, 50000)
TEST_SYM_INTERVAL (V_NAME_F1 (exp10m1), 0x1p-1, 0x1p0, 5000)
TEST_SYM_INTERVAL (V_NAME_F1 (exp10m1), 0x1p0, 0x1p1, 5000)
TEST_SYM_INTERVAL (V_NAME_F1 (exp10m1), 0x1p1, 0x1p8, 5000)
#endif
