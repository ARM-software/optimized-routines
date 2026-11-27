/*
 * Helper 2^n routine for double precision exponentials.
 *
 * Copyright (c) 2026, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#ifndef MATH_V_EXP_SPECIAL_INLINE_H
#define MATH_V_EXP_SPECIAL_INLINE_H

#include "v_math.h"

static const struct v_exp_special_data
{
  uint64x2_t special_offset, special_bias1, special_bias2;
} V_EXP_SPECIAL_DATA = {
  .special_offset = V2 (0x6000000000000000), /* 0x1p513.  */
  .special_bias1 = V2 (0x7000000000000000),  /* 0x1p769.  */
  .special_bias2 = V2 (0x3010000000000000),  /* 0x1p-254.  */
};

static inline float64x2_t VPCS_ATTR
exp_special (float64x2_t poly, float64x2_t n, float64x2_t scale,
	     float64x2_t scale_bound, const struct v_exp_special_data *ds)
{
  /* 2^n may overflow, break it up into s1*s2.  */
  uint64x2_t b = vandq_u64 (vclezq_f64 (n), ds->special_offset);
  float64x2_t s1 = vreinterpretq_f64_u64 (vsubq_u64 (ds->special_bias1, b));
  float64x2_t s2 = vreinterpretq_f64_u64 (vaddq_u64 (
      vsubq_u64 (vreinterpretq_u64_f64 (scale), ds->special_bias2), b));
  uint64x2_t cmp2 = vcagtq_f64 (n, scale_bound);
  float64x2_t r1 = vmulq_f64 (s1, s1);
  float64x2_t r2 = vmulq_f64 (vfmaq_f64 (s2, s2, poly), s1);
  return vbslq_f64 (cmp2, r1, r2);
}

#endif // MATH_V_EXP_SPECIAL_INLINE_H
