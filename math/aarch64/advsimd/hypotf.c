/*
 * Single-precision vector hypot(x) function.
 *
 * Copyright (c) 2023-2026, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "v_math.h"
#include "test_sig.h"
#include "test_defs.h"

static const struct data
{
  uint32x4_t tiny_bound;
  uint32x4_t thres;
  float32x4_t scale;
  float32x4_t invScale;
  float32x4_t scale_up_thres;
  float32x4_t scale_down_thres;
  float32x4_t inf;
} data = {
  .tiny_bound = V4 (0x0C800000), /* asuint (0x1p-102).  */
  .thres = V4 (0x73000000),
  /* 75 is the minimal symmetric exponent such that all single-precision
    inputs, including subnormals, can be handled safely. The smallest positive
    float is 2^-149. When scaling inputs by 2^75 before squaring, we get:
      (2^-149 · 2^75)^2 = 2^-148
    which is representable as a subnormal float.

    Therefore, scale = 2^75 (and invScale = 2^-75) is the smallest power-of-two
    scaling that prevents underflow in x*x + y*y while remaining symmetric for
    scaling back the final result.  */
  .scale = V4 (0x1p-75f),
  .invScale = V4 (0x1p+75f),
  .scale_up_thres = V4 (0x1p+60f),
  .scale_down_thres = V4 (0x1p-60f),
  .inf = V4 (INFINITY),
};

static float32x4_t VPCS_ATTR inline kernel (float32x4_t ax, float32x4_t ay,
					    const struct data *d)
{
  /* Kernel is a function written to choose the algebric formulation that gives
    the smaller rounding error for the current magnitude ratio. Kernel returns
    a^2 + b^2 not the hypot itself this can be returned as:
    - sum1 = (a-b)^2 + (2*b)*a where t1=2*b and t2=a-b
    - sum2 which is just a^2 + b^2.  */
  float32x4_t t1 = vaddq_f32 (ay, ay);
  float32x4_t t2 = vsubq_f32 (ax, ay);
  float32x4_t sum1 = vfmaq_f32 (vmulq_f32 (t2, t2), t1, ax);
  float32x4_t sum2 = vfmaq_f32 (vmulq_f32 (ay, ay), ax, ax);
  uint32x4_t mask = vcgeq_f32 (t1, ax);
  return vbslq_f32 (mask, sum1, sum2);
}

static float32x4_t VPCS_ATTR inline special_case (uint32x4_t special,
						  float32x4_t sqsum,
						  float32x4_t x, float32x4_t y,
						  const struct data *d)
{
  float32x4_t ax = vabsq_f32 (x);
  float32x4_t ay = vabsq_f32 (y);
  float32x4_t amin = vminq_f32 (ax, ay);
  float32x4_t amax = vmaxq_f32 (ax, ay);
  uint32x4_t mask_big = vcgtq_f32 (amax, d->scale_up_thres);
  uint32x4_t mask_tiny = vcltq_f32 (amin, d->scale_down_thres);

  /* Implemented vectorised fallback for AdvSIMD hypotf special case
    for large and tiny values by avoiding underflow/overflow caused
    by squaring the inputs.  */
  float32x4_t scale = v_f32 (1.0f);
  float32x4_t inv_scale = v_f32 (1.0f);
  scale = vbslq_f32 (mask_tiny, d->invScale, scale);
  scale = vbslq_f32 (mask_big, d->scale, scale);
  inv_scale = vbslq_f32 (mask_tiny, d->scale, inv_scale);
  inv_scale = vbslq_f32 (mask_big, d->invScale, inv_scale);
  x = vmulq_f32 (amax, scale);
  y = vmulq_f32 (amin, scale);
  float32x4_t res = vmulq_f32 (vsqrtq_f32 (kernel (x, y, d)), inv_scale);

  /* Handles cases where either x or y are not finite, returning infinity when
    needed. Vminq & vmaxq will return NaN whenever one parameter is NaN. We
    add an extra 'inf_mask' to make sure infinity is returned when either
    absolute x or absolute y are infinity and the other value is NaN.

    This also handles cases of x = inf and y = inf, which will become NaN when
    subtracted in the kernel function, using vsubq.  */
  uint32x4_t inf_mask
      = vorrq_u32 (vceqq_f32 (ax, d->inf), vceqq_f32 (ay, d->inf));
  res = vbslq_f32 (inf_mask, d->inf, res);

  return vbslq_f32 (special, res, vsqrtq_f32 (sqsum));
}

/* Vector implementation of single-precision hypot.
   Maximum error observed is 1.21 ULP:
   _ZGVnN4vv_hypotf (0x1.6a419cp-13, 0x1.82a852p-22) got 0x1.6a41d2p-13
						    want 0x1.6a41dp-13.  */
float32x4_t VPCS_ATTR NOINLINE V_NAME_F2 (hypot) (float32x4_t x, float32x4_t y)
{
  const struct data *d = ptr_barrier (&data);
  float32x4_t sqsum = vfmaq_f32 (vmulq_f32 (x, x), y, y);

  uint32x4_t special = vcgeq_u32 (
      vsubq_u32 (vreinterpretq_u32_f32 (sqsum), d->tiny_bound), d->thres);

  if (unlikely (v_any_u32 (special)))
    return special_case (special, sqsum, x, y, d);
  return vsqrtq_f32 (sqsum);
}

HALF_WIDTH_ALIAS_F2 (hypot)

TEST_SIG (V, F, 2, hypot, -10.0, 10.0)
TEST_ULP (V_NAME_F2 (hypot), 0.71)
TEST_INTERVAL2 (V_NAME_F2 (hypot), 0, inf, 0, inf, 10000)
TEST_INTERVAL2 (V_NAME_F2 (hypot), 0, inf, -0, -inf, 10000)
TEST_INTERVAL2 (V_NAME_F2 (hypot), -0, -inf, 0, inf, 10000)
TEST_INTERVAL2 (V_NAME_F2 (hypot), -0, -inf, -0, -inf, 10000)
TEST_INTERVAL2 (V_NAME_F2 (hypot), -inf, inf, -inf, inf, 10000)
TEST_INTERVAL2 (V_NAME_F2 (hypot), inf, inf, inf, inf, 1)
TEST_INTERVAL2 (V_NAME_F2 (hypot), inf, inf, nan, nan, 1)
TEST_INTERVAL2 (V_NAME_F2 (hypot), nan, nan, inf, inf, 1)
TEST_INTERVAL2 (V_NAME_F2 (hypot), nan, nan, nan, nan, 1)
