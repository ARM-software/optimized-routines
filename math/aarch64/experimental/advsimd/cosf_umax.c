/*
 * Low accuracy single-precision vector cos function.
 *
 * Copyright (c) 2025, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "mathlib.h"
#include "v_math.h"
#include "test_defs.h"

static const struct data
{
  float32x4_t range_val, c0;
  float c1, pi_1, pi_2, inv_pi;
} data = {
  .pi_1 = 0x1.921fb6p+1f,
  .pi_2 = -0x1.777a5cp-24f,
  .inv_pi = 0x1.45f306p-2f,
  /* Coefficients obtained with iterative Remez approach.  */
  .c0 = V4 (-0x1.545304p-3f),
  .c1 = 0x1.f73186p-8f,
  /* The scalar fallback is still required for large values.
     The threshold is adapted to maintain accuracy to 4096 ULP.  */
  .range_val = V4 (0x1p15f),
};

static float32x4_t VPCS_ATTR NOINLINE
special_case (float32x4_t x, float32x4_t y, uint32x4_t odd, uint32x4_t cmp)
{
  /* Fall back to scalar code.  */
  y = vreinterpretq_f32_u32 (veorq_u32 (vreinterpretq_u32_f32 (y), odd));
  return v_call_f32 (cosf, x, y, cmp);
}

/* Fast inaccurate version of single-precision cos.
   Maximum error: 2857.36 +0.5 ULP
   arm_math_advsimd_fast_cosf(0x1.96586ap+14) got 0x1.000b28p+0
					     want 0x1.fffffcp-1.  */
float32x4_t VPCS_ATTR NOINLINE
arm_math_advsimd_fast_cosf (float32x4_t x)
{
  const struct data *d = ptr_barrier (&data);

  /* Branch is necessary to maintain accuracy on large values of |x|.  */
  uint32x4_t cmp = vcageq_f32 (x, d->range_val);

  float32x4_t cts = vld1q_f32 (&d->c1);

  /* n = rint((|x|+pi/2)/pi) - 0.5.  */
  float32x4_t n = vrndaq_f32 (vfmaq_laneq_f32 (v_f32 (0.5f), x, cts, 3));
  uint32x4_t odd = vshlq_n_u32 (vreinterpretq_u32_s32 (vcvtq_s32_f32 (n)), 31);
  n = vsubq_f32 (n, v_f32 (0.5f));

  /* r = |x| - n*pi  (range reduction into -pi/2 .. pi/2).  */
  float32x4_t r = x;
  r = vfmsq_laneq_f32 (r, n, cts, 1);
  r = vfmsq_laneq_f32 (r, n, cts, 2);

  /* y = sin(r).  */
  float32x4_t r2 = vmulq_f32 (r, r);
  float32x4_t r3 = vmulq_f32 (r2, r);
  float32x4_t y = vfmaq_laneq_f32 (d->c0, r2, cts, 0);
  y = vfmaq_f32 (r, r3, y);

  if (unlikely (v_any_u32 (cmp)))
    return special_case (x, y, odd, cmp);
  return vreinterpretq_f32_u32 (veorq_u32 (vreinterpretq_u32_f32 (y), odd));
}

TEST_ULP (arm_math_advsimd_fast_cosf, 4096)
TEST_DISABLE_FENV (arm_math_advsimd_fast_cosf)
TEST_SYM_INTERVAL (arm_math_advsimd_fast_cosf, 0, 0x1p15f, 500000)
TEST_SYM_INTERVAL (arm_math_advsimd_fast_cosf, 0x1p15f, inf, 10000)
