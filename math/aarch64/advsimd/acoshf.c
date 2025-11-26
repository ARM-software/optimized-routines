/*
 * Single-precision vector acosh(x) function.
 * Copyright (c) 2023-2025, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "v_math.h"
#include "test_sig.h"
#include "test_defs.h"
#include "v_log1pf_inline.h"

const static struct data
{
  struct v_log1pf_data log1pf_consts;
  uint32x4_t one;
  uint16x4_t special_bound_u16;
  uint32x4_t special_bound_u32;
  float32x4_t pinf, nan;
} data = {
  .log1pf_consts = V_LOG1PF_CONSTANTS_TABLE,
  .one = V4 (0x3f800000),
  .special_bound_u16 = V4 (0x2000),
  /* asuint(sqrt(FLT_MAX)) - asuint(1).  */
  .special_bound_u32 = V4 (0x20000000),
  .pinf = V4 (INFINITY),
  .nan = V4 (NAN),
};

static inline float32x4_t VPCS_ATTR
inline_acoshf (float32x4_t x, const struct data *d)
{
  /* acosh(x) = ln(x + sqrt[x^2 -1]).
    So acosh(x) = log1p (x + sqrt[x^2 - 1] - 1).  */
  float32x4_t xm1 = vsubq_f32 (x, vreinterpretq_f32_u32 (d->one));
  float32x4_t u
      = vmulq_f32 (xm1, vaddq_f32 (x, vreinterpretq_f32_u32 (d->one)));

  float32x4_t y = vaddq_f32 (xm1, vsqrtq_f32 (u));

  return log1pf_inline (y, &d->log1pf_consts);
}

static float32x4_t VPCS_ATTR NOINLINE
special_case (float32x4_t x, const struct data *d)
{
  uint32x4_t special = vcgeq_u32 (
      vsubq_u32 (vreinterpretq_u32_f32 (x), d->one), d->special_bound_u32);

  /* To avoid the overflow in x^2 (so the x < sqrt(FLT_MAX) constraint), we
    reduce the input of acosh to a narrower interval by relying on the identity
    acosh(t) = 1/2acosh(2t^2 - 1) for t>=1.
    If we set t=sqrt((x+1)/2), since x>=1 then t>=sqrt(2/2)=1, and therefore
    acosh(x) = 2acosh(sqrt((x+1)/2)).  */
  float32x4_t r = vaddq_f32 (x, vreinterpretq_f32_u32 (d->one));
  r = vmulq_f32 (r, v_f32 (0.5f));
  r = vbslq_f32 (special, vsqrtq_f32 (r), x);

  float32x4_t y = inline_acoshf (r, d);

  y = vbslq_f32 (special, vmulq_f32 (y, v_f32 (2.0f)), y);

  /* Check whether x is less than 1, or x is inf or nan.  */
  uint32x4_t inf_minus_one
      = vsubq_u32 (vreinterpretq_u32_f32 (d->pinf), d->one);
  uint32x4_t is_infnan = vcgeq_u32 (
      vsubq_u32 (vreinterpretq_u32_f32 (x), d->one), inf_minus_one);

  y = vbslq_f32 (is_infnan, d->nan, y);
  uint32x4_t ret_pinf = vceqq_f32 (x, d->pinf);
  y = vbslq_f32 (ret_pinf, d->pinf, y);
  return y;
}

/* Vector approximation for single-precision acosh, based on log1p.
   The largest observed error is 3.22 ULP:
   _ZGVnN4v_acoshf(0x1.007ef2p+0) got 0x1.fdcdccp-5
				 want 0x1.fdcdd2p-5.  */

float32x4_t VPCS_ATTR NOINLINE V_NAME_F1 (acosh) (float32x4_t x)
{
  const struct data *d = ptr_barrier (&data);
  uint32x4_t ix = vreinterpretq_u32_f32 (x);
  /* Inputs greater than or equal to special_bound will cause the output to
    overflow. This is because there is a square operation in log1pf_inline.
    This also captures inf, nan and any input less than or equal to 1.  */
  uint16x4_t special
      = vcge_u16 (vsubhn_u32 (ix, d->one), d->special_bound_u16);

  if (unlikely (v_any_u16h (special)))
    return special_case (x, d);
  return inline_acoshf (x, d);
}

HALF_WIDTH_ALIAS_F1 (acosh)

TEST_SIG (V, F, 1, acosh, 1.0, 10.0)
TEST_ULP (V_NAME_F1 (acosh), 2.72)
TEST_INTERVAL (V_NAME_F1 (acosh), 0, 1, 500)
TEST_INTERVAL (V_NAME_F1 (acosh), 1, 0x1p64, 100000)
TEST_INTERVAL (V_NAME_F1 (acosh), 0x1p64, inf, 1000)
TEST_INTERVAL (V_NAME_F1 (acosh), -0, -inf, 1000)
