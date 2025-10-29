/*
 * Single-precision vector log(1+x) function.
 *
 * Copyright (c) 2022-2025, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "v_math.h"
#include "test_sig.h"
#include "test_defs.h"
#include "v_log1pf_inline.h"

static const struct data
{
  struct v_log1pf_data d;
  float32x4_t nan, pinf, minf;
} data = {
  .d = V_LOG1PF_CONSTANTS_TABLE,
  .nan = V4 (NAN),
  .pinf = V4 (INFINITY),
  .minf = V4 (-INFINITY),
};

static inline float32x4_t
special_case (float32x4_t x, uint32x4_t cmp, const struct data *d)
{
  float32x4_t y = log1pf_inline (x, ptr_barrier (&d->d));
  y = vbslq_f32 (cmp, d->nan, y);
  uint32x4_t ret_pinf = vceqq_f32 (x, d->pinf);
  uint32x4_t ret_minf = vceqq_f32 (x, v_f32 (-1.0));

  y = vbslq_f32 (ret_pinf, d->pinf, y);
  return vbslq_f32 (ret_minf, d->minf, y);
}

/* Single-precision implementation of vector log1pf(x).
  Maximum observed error: 1.20 + 0.5
  _ZGVnN4v_log1pf(0x1.04418ap-2) got 0x1.cfcbd8p-3
				want 0x1.cfcbdcp-3.  */
float32x4_t VPCS_ATTR NOINLINE V_NAME_F1 (log1p) (float32x4_t x)
{
  const struct data *d = ptr_barrier (&data);

  /* Use signed integers here to ensure that negative numbers between 0 and -1
    don't make this expression true.  */
  uint32x4_t is_infnan
      = vcgeq_s32 (vreinterpretq_s32_f32 (x), vreinterpretq_s32_f32 (d->pinf));
  /* The OR-NOT is needed to catch -NaN.  */
  uint32x4_t special = vornq_u32 (is_infnan, vcgtq_f32 (x, v_f32 (-1)));

  if (unlikely (v_any_u32 (special)))
    return special_case (x, special, d);

  return log1pf_inline (x, &d->d);
}

HALF_WIDTH_ALIAS_F1 (log1p)

TEST_SIG (V, F, 1, log1p, -0.9, 10.0)
TEST_ULP (V_NAME_F1 (log1p), 1.20)
TEST_SYM_INTERVAL (V_NAME_F1 (log1p), 0.0, 0x1p-23, 30000)
TEST_SYM_INTERVAL (V_NAME_F1 (log1p), 0x1p-23, 1, 50000)
TEST_INTERVAL (V_NAME_F1 (log1p), 1, inf, 50000)
TEST_INTERVAL (V_NAME_F1 (log1p), -1.0, -inf, 1000)
