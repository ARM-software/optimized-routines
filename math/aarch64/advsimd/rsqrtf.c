/*
 * Single-precision vector rsqrt(x) function.
 *
 * Copyright (c) 2025, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "test_defs.h"
#include "v_math.h"

static const struct data
{
  float32x4_t special_bound;
  float32x4_t scale_up, scale_down;
} data = {
  /* When x < 0x1p-128, estimate becomes infinity.
    x is scaled up by 0x1p22f, so estimate does not reach infinity.
    Then the result is multiplied by 0x1p11f.
    The difference between the lowest power possible (-149) and the special
    bound (-128) is 21. 22 is used here so that a power of 2 can be used for
    scaling in both directions.  */
  .special_bound = V4 (0x1p-128f),
  .scale_up = V4 (0x1p22f),
  .scale_down = V4 (0x1p11f),
};

static inline float32x4_t VPCS_ATTR
inline_rsqrt (float32x4_t x)
{
  /* Do estimate instruction.  */
  float32x4_t estimate = vrsqrteq_f32 (x);

  /* Do first step instruction.  */
  float32x4_t estimate_squared = vmulq_f32 (estimate, estimate);
  float32x4_t step = vrsqrtsq_f32 (x, estimate_squared);
  estimate = vmulq_f32 (estimate, step);

  /* Do second step instruction.
    This is required to achieve < 3.0 ULP.  */
  estimate_squared = vmulq_f32 (estimate, estimate);
  step = vrsqrtsq_f32 (x, estimate_squared);
  estimate = vmulq_f32 (estimate, step);
  return estimate;
}

static float32x4_t NOINLINE
special_case (float32x4_t x, uint32x4_t special, const struct data *d)
{
  x = vbslq_f32 (special, vmulq_f32 (x, d->scale_up), x);
  float32x4_t estimate = inline_rsqrt (x);
  return vbslq_f32 (special, vmulq_f32 (estimate, d->scale_down), estimate);
}

/* Single-precision implementation of vector rqsrtf(x).
  Maximum observed error: 1.47 + 0.5
  _ZGVnN4v_rsqrtf (0x1.f610dep+127) got 0x1.02852cp-64
				   want 0x1.02853p-64.  */
float32x4_t VPCS_ATTR NOINLINE V_NAME_F1 (rsqrt) (float32x4_t x)
{
  const struct data *d = ptr_barrier (&data);

  /* Special case: x < special_bound.  */
  uint32x4_t special = vcgtq_f32 (d->special_bound, x);
  if (unlikely (v_any_u32 (special)))
    {
      return special_case (x, special, d);
    }
  return inline_rsqrt (x);
}
HALF_WIDTH_ALIAS_F1 (rsqrt);

#if WANT_C23_TESTS
TEST_ULP (V_NAME_F1 (rsqrt), 1.97)
TEST_SYM_INTERVAL (V_NAME_F1 (rsqrt), 0, 1, 5000)
TEST_SYM_INTERVAL (V_NAME_F1 (rsqrt), 1, 100, 5000)
TEST_INTERVAL (V_NAME_F1 (rsqrt), 0, 0x1p-128f, 5000)
TEST_INTERVAL (V_NAME_F1 (rsqrt), 100, inf, 5000)
TEST_INTERVAL (V_NAME_F1 (rsqrt), -100, -inf, 5000)
#endif