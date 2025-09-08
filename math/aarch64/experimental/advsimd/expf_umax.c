/*
 * Low accuracy single-precision vector e^x function.
 *
 * Copyright (c) 2025, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */
#include "v_math.h"
#include "test_defs.h"

static const struct data
{
  float32x4_t c0, c1;
  float inv_ln2, ln2, c2, null;
  float32x4_t special_bound;
  uint32x4_t exponent_bias;
  /* Special case routine.  */
  uint32x4_t special_offset, special_bias;
  float32x4_t scale_thresh;
} data = {
  /* exp(x) - 1 ~ r * (c0 + r * c1 + r * c2).  */
  .c0 = V4 (0x1.fff970p-1f),
  .c1 = V4 (0x1.022158p-1f),
  .c2 = 0x1.583e32p-3f,
  .inv_ln2 = 0x1.715476p+0f,
  .ln2 = 0x1.62e43p-1f,
  .exponent_bias = V4 (0x3f800000),
  /* Lower the bound to maintain accuracy on negative values.  */
  .special_bound = V4 (125.0f),
  /* Special case.  */
  .special_offset = V4 (0x82000000),
  .special_bias = V4 (0x7f000000),
  .scale_thresh = V4 (192.0f),
};

static float32x4_t VPCS_ATTR NOINLINE
special_case (float32x4_t poly, float32x4_t n, uint32x4_t e, uint32x4_t cmp1,
	      float32x4_t scale, const struct data *d)
{
  /* 2^n may overflow, break it up into s1 * s2.  */
  uint32x4_t b = vandq_u32 (vclezq_f32 (n), d->special_offset);
  float32x4_t s1 = vreinterpretq_f32_u32 (vaddq_u32 (b, d->special_bias));
  float32x4_t s2 = vreinterpretq_f32_u32 (vsubq_u32 (e, b));
  uint32x4_t cmp2 = vcagtq_f32 (n, d->scale_thresh);
  float32x4_t r2 = vmulq_f32 (s1, s1);
  /* (s2 + p * s2) * s1 = s2 * (p+1) * s1.  */
  float32x4_t r1 = vmulq_f32 (vfmaq_f32 (s2, poly, s2), s1);
  /* Similar to r1 but avoids double rounding in the subnormal range.  */
  float32x4_t r0 = vfmaq_f32 (scale, poly, scale);
  float32x4_t r = vbslq_f32 (cmp1, r1, r0);
  return vbslq_f32 (cmp2, r2, r);
}

/* Low accuracy AdvSIMD expf
   Maximum error: 1741.71 +0.5 ULP
   arm_math_advsimd_fast_expf(-0x1.5b7322p+6) got 0x1.9b56bep-126
					     want 0x1.9b491cp-126.  */
float32x4_t VPCS_ATTR NOINLINE
arm_math_advsimd_fast_expf (float32x4_t x)
{
  const struct data *d = ptr_barrier (&data);
  float32x4_t ln2_c2 = vld1q_f32 (&d->inv_ln2);

  /* exp(x) = 2^n (1 + poly(r)), with 1 + poly(r) in [1/sqrt(2),sqrt(2)]
     x = ln2*n + r, with r in [-ln2/2, ln2/2].  */
  float32x4_t n = vrndaq_f32 (vmulq_laneq_f32 (x, ln2_c2, 0));
  float32x4_t r = vfmsq_laneq_f32 (x, n, ln2_c2, 1);
  uint32x4_t e = vshlq_n_u32 (vreinterpretq_u32_s32 (vcvtq_s32_f32 (n)), 23);

  float32x4_t poly;
  poly = vfmaq_laneq_f32 (d->c1, r, ln2_c2, 2);
  poly = vfmaq_f32 (d->c0, r, poly);

  uint32x4_t cmp = vcagtq_f32 (n, d->special_bound);
  if (unlikely (v_any_u32 (cmp)))
    {
      poly = vmulq_f32 (poly, r);
      float32x4_t scale
	  = vreinterpretq_f32_u32 (vaddq_u32 (e, d->exponent_bias));
      return special_case (poly, n, e, cmp, scale, d);
    }
  /* For smaller values simply offset exponent, instead of using flops.  */
  poly = vfmaq_f32 (v_f32 (1.0f), r, poly);
  return vreinterpretq_f32_u32 (vaddq_u32 (vreinterpretq_u32_f32 (poly), e));
}

TEST_ULP (arm_math_advsimd_fast_expf, 4096)
TEST_SYM_INTERVAL (arm_math_advsimd_fast_expf, 0.0, 125.0f, 50000)
TEST_SYM_INTERVAL (arm_math_advsimd_fast_expf, 125.0f, inf, 50000)
