/*
 * Single-precision vector atanh(x) function.
 *
 * Copyright (c) 2022-2025, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "v_math.h"
#include "test_sig.h"
#include "test_defs.h"
#include "v_log1pf_inline.h"

const static struct data
{
  struct v_log1pf_data log1pf_consts;
  uint32x4_t abs_mask;
  float32x4_t half;
  uint32x4_t one;
  float32x4_t pinf, minf, nan;
} data = {
  .log1pf_consts = V_LOG1PF_CONSTANTS_TABLE,
  .abs_mask = V4 (0x7fffffff),
  .half = V4 (0.5f),
  .one = V4 (0x3f800000),
  .pinf = V4 (INFINITY),
  .minf = V4 (-INFINITY),
  .nan = V4 (NAN),
};

static float32x4_t VPCS_ATTR NOINLINE
special_case (float32x4_t x, float32x4_t y, float32x4_t halfsign,
	      uint32x4_t special, const struct data *d)
{
  y = log1pf_inline (y, &d->log1pf_consts);
  y = vmulq_f32 (halfsign, y);

  float32x4_t float_one = vreinterpretq_f32_u32 (d->one);
  /* Need the NOT here to make an input of NaN return true.  */
  uint32x4_t ret_nan = vmvnq_u32 (vcaltq_f32 (x, float_one));
  uint32x4_t ret_pinf = vceqzq_f32 (vsubq_f32 (x, float_one));
  uint32x4_t ret_minf = vceqzq_f32 (vaddq_f32 (x, float_one));

  y = vbslq_f32 (ret_nan, d->nan, y);
  y = vbslq_f32 (ret_pinf, d->pinf, y);
  y = vbslq_f32 (ret_minf, d->minf, y);
  return y;
}

/* Approximation for vector single-precision atanh(x) using modified log1p.
   The maximum error is 2.93 ULP:
   _ZGVnN4v_atanhf(0x1.f43d7p-5) got 0x1.f4dcfep-5
				want 0x1.f4dcf8p-5.  */
float32x4_t VPCS_ATTR NOINLINE V_NAME_F1 (atanh) (float32x4_t x)
{
  const struct data *d = ptr_barrier (&data);

  /* This computes a 1/2 with the sign bit being the sign bit of x.
    This is because:
    atanh(x) = 1/2 ln ((1+x)/(1-x))
    = 1/2 log1pf ((1+x)/(1-x) - 1)
    = 1/2 log1pf (2x/(1-x)).  */
  float32x4_t halfsign = vbslq_f32 (d->abs_mask, d->half, x);
  float32x4_t ax = vabsq_f32 (x);
  uint32x4_t iax = vreinterpretq_u32_f32 (ax);

  /* Values between but not including -1 and 1 are treated appropriately by the
    fast pass. -1 and 1 are asymptotes of atanh(x), so they are treated as
    special cases along with any input outside of these bounds.  */
  uint32x4_t special = vcgeq_u32 (iax, d->one);

  float32x4_t r = vdivq_f32 (vaddq_f32 (ax, ax),
			     vsubq_f32 (vreinterpretq_f32_u32 (d->one), ax));

  if (unlikely (v_any_u32 (special)))
    return special_case (x, r, halfsign, special, d);

  float32x4_t y = log1pf_inline (r, &d->log1pf_consts);
  return vmulq_f32 (halfsign, y);
}

HALF_WIDTH_ALIAS_F1 (atanh)

TEST_SIG (V, F, 1, atanh, -1.0, 1.0)
TEST_ULP (V_NAME_F1 (atanh), 2.44)
TEST_SYM_INTERVAL (V_NAME_F1 (atanh), 0, 0x1p-12, 500)
TEST_SYM_INTERVAL (V_NAME_F1 (atanh), 0x1p-12, 1, 200000)
TEST_SYM_INTERVAL (V_NAME_F1 (atanh), 1, inf, 1000)
/* atanh is asymptotic at 1, which is the default control value - have to set
 -c 0 specially to ensure fp exceptions are triggered correctly (choice of
 control lane is irrelevant if fp exceptions are disabled).  */
TEST_CONTROL_VALUE (V_NAME_F1 (atanh), 0)
