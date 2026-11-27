/*
 * Double-precision vector hypot(x) function.
 *
 * Copyright (c) 2023-2026, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "v_math.h"
#include "test_sig.h"
#include "test_defs.h"

static const struct data
{
  uint64x2_t tiny_bound;
  uint64x2_t thres64;
  float64x2_t scale;
  float64x2_t inv_scale;
  float64x2_t scale_up_thres;
  float64x2_t scale_down_thres;
  float64x2_t inf;
} data
    = { .tiny_bound = V2 (0x0360000000000000), /* asuint (0x1p-969).  */
	.thres64 = V2 (0x7c90000000000000), /* asuint (inf) - tiny_bound.  */
	/* 537 is the minimal symmetric exponent such that all
	  double-precision inputs, including subnormals, can be handled
	  safely. The smallest positive float is 2^-1074. When scaling
	  inputs by 2^537 before squaring, we get: (2^-149 · 2^75)^2 =
	  2^-148 which is representable as a subnormal float.

	  Therefore, scale = 2^537 (and invScale = 2^-537) is the smallest
	  power-of-two scaling that prevents underflow in x*x + y*y while
	  remaining symmetric for scaling back the final result.  */
	.scale = V2 (0x1p-537),
	.inv_scale = V2 (0x1p+537),
	.scale_up_thres = V2 (0x1p+511),
	.scale_down_thres = V2 (0x1p-459),
	.inf = V2 (INFINITY) };

static float64x2_t VPCS_ATTR inline kernel (float64x2_t ax, float64x2_t ay,
					    const struct data *d)
{
  /* Kernel is a function written to choose the algebric formulation that gives
    the smaller rounding error for the current magnitude ratio. Kernel returns
    a^2 + b^2 not the hypot itself this can be returned as:
    - sum1 = (a-b)^2 + (2*b)*a where t1=2*b and t2=a-b
    - sum2 which is just a^2 + b^2.  */
  float64x2_t t1 = vaddq_f64 (ay, ay);
  float64x2_t t2 = vsubq_f64 (ax, ay);
  float64x2_t sum1 = vfmaq_f64 (t2 * t2, t1, ax);
  float64x2_t sum2 = vfmaq_f64 (ay * ay, ax, ax);
  uint64x2_t mask = vcgeq_f64 (t1, ax);
  return vbslq_f64 (mask, sum1, sum2);
}

static float64x2_t VPCS_ATTR inline special_case (uint64x2_t special,
						  float64x2_t sqsum,
						  float64x2_t x, float64x2_t y,
						  const struct data *d)
{
  float64x2_t ax = vabsq_f64 (x);
  float64x2_t ay = vabsq_f64 (y);
  float64x2_t amin = vminq_f64 (ax, ay);
  float64x2_t amax = vmaxq_f64 (ax, ay);
  uint64x2_t mask_big = vcgtq_f64 (amax, d->scale_up_thres);
  uint64x2_t mask_tiny = vcltq_f64 (amin, d->scale_down_thres);

  /* Implemented vectorised fallback for AdvSIMD hypotf special case
    for large and tiny values by avoiding underflow/overflow caused
    by squaring the inputs.  */
  float64x2_t scale = v_f64 (1.0f);
  float64x2_t inv_scale = v_f64 (1.0f);
  scale = vbslq_f64 (mask_tiny, d->inv_scale, scale);
  scale = vbslq_f64 (mask_big, d->scale, scale);
  inv_scale = vbslq_f64 (mask_tiny, d->scale, inv_scale);
  inv_scale = vbslq_f64 (mask_big, d->inv_scale, inv_scale);
  x = vmulq_f64 (amax, scale);
  y = vmulq_f64 (amin, scale);
  float64x2_t res = vmulq_f64 (vsqrtq_f64 (kernel (x, y, d)), inv_scale);

  /* Handles cases where either x or y are not finite, returning infinity when
    needed. Vminq & vmaxq will return NaN whenever one parameter is NaN. We
    add an extra 'inf_mask' to make sure infinity is returned when either
    absolute x or absolute y are infinity and the other value is NaN.

    This also handles cases of x = inf and y = inf, which will become NaN when
    subtracted in the kernel function, using vsubq.  */
  uint64x2_t inf_mask
      = vorrq_u64 (vceqq_f64 (ax, d->inf), vceqq_f64 (ay, d->inf));
  res = vbslq_f64 (inf_mask, d->inf, res);

  return vbslq_f64 (special, res, vsqrtq_f64 (sqsum));
}

/* Vector implementation of double-precision hypot.
   Maximum error observed is 1.21 ULP:
   _ZGVnN2vv_hypot (0x1.6a1b193ff85b5p-204, 0x1.bc50676c2a447p-222)
    got 0x1.6a1b19400964ep-204
   want 0x1.6a1b19400964dp-204.  */
float64x2_t VPCS_ATTR V_NAME_D2 (hypot) (float64x2_t x, float64x2_t y)
{
  const struct data *d = ptr_barrier (&data);
  float64x2_t sqsum = vfmaq_f64 (vmulq_f64 (x, x), y, y);

  uint64x2_t special = vcgeq_u64 (
      vsubq_u64 (vreinterpretq_u64_f64 (sqsum), d->tiny_bound), d->thres64);

  if (unlikely (v_any_u64 (special)))
    return special_case (special, sqsum, x, y, d);

  return vsqrtq_f64 (sqsum);
}

TEST_SIG (V, D, 2, hypot, -10.0, 10.0)
TEST_ULP (V_NAME_D2 (hypot), 0.71)
TEST_INTERVAL2 (V_NAME_D2 (hypot), 0, inf, 0, inf, 10000)
TEST_INTERVAL2 (V_NAME_D2 (hypot), 0, inf, -0, -inf, 10000)
TEST_INTERVAL2 (V_NAME_D2 (hypot), -0, -inf, 0, inf, 10000)
TEST_INTERVAL2 (V_NAME_D2 (hypot), -0, -inf, -0, -inf, 10000)
TEST_INTERVAL2 (V_NAME_F2 (hypot), inf, inf, inf, inf, 1)
TEST_INTERVAL2 (V_NAME_F2 (hypot), inf, inf, nan, nan, 1)
TEST_INTERVAL2 (V_NAME_F2 (hypot), nan, nan, inf, inf, 1)
TEST_INTERVAL2 (V_NAME_F2 (hypot), nan, nan, nan, nan, 1)
