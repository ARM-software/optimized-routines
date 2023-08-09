/*
 * Helper for single-precision routines which calculate log(1 + x) and do not
 * need special-case handling
 *
 * Copyright (c) 2022-2023, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#ifndef PL_MATH_V_LOG1PF_INLINE_H
#define PL_MATH_V_LOG1PF_INLINE_H

#include "v_math.h"

const static struct v_log1pf_data
{
  float32x4_t poly[8], ln2;
  uint32x4_t tiny_bound, minus_one, four, thresh;
  uint32x4_t three_quarters;
} v_log1pf_data = {
  .poly = { /* Generated using FPMinimax in [-0.25, 0.5]. First two coefficients
	       (1, -0.5) are not stored as they can be generated more
	       efficiently.  */
	    V4 (0x1.5555aap-2f), V4 (-0x1.000038p-2f), V4 (0x1.99675cp-3f),
	    V4 (-0x1.54ef78p-3f), V4 (0x1.28a1f4p-3f), V4 (-0x1.0da91p-3f),
	    V4 (0x1.abcb6p-4f), V4 (-0x1.6f0d5ep-5f) },
  .ln2 = V4 (0x1.62e43p-1f),
  .tiny_bound = V4 (0x34000000), /* asuint32(0x1p-23). ulp=0.5 at 0x1p-23.  */
  .thresh = V4 (0x4b800000), /* asuint32(INFINITY) - tiny_bound.  */
  .minus_one = V4 (0xbf800000),
  .four = V4 (0x40800000),
  .three_quarters = V4 (0x3f400000)
};

static inline float32x4_t
eval_poly (float32x4_t m, const float32x4_t *p)
{
  /* Approximate log(1+m) on [-0.25, 0.5] using split Estrin scheme.  */
  float32x4_t p_12 = vfmaq_f32 (v_f32 (-0.5), m, p[0]);
  float32x4_t p_34 = vfmaq_f32 (p[1], m, p[2]);
  float32x4_t p_56 = vfmaq_f32 (p[3], m, p[4]);
  float32x4_t p_78 = vfmaq_f32 (p[5], m, p[6]);

  float32x4_t m2 = vmulq_f32 (m, m);
  float32x4_t p_02 = vfmaq_f32 (m, m2, p_12);
  float32x4_t p_36 = vfmaq_f32 (p_34, m2, p_56);
  float32x4_t p_79 = vfmaq_f32 (p_78, m2, p[7]);

  float32x4_t m4 = vmulq_f32 (m2, m2);
  float32x4_t p_06 = vfmaq_f32 (p_02, m4, p_36);
  return vfmaq_f32 (p_06, m4, vmulq_f32 (m4, p_79));
}

static inline float32x4_t
log1pf_inline (float32x4_t x)
{
  const struct v_log1pf_data *d = ptr_barrier (&v_log1pf_data);
  /* Helper for calculating log(x + 1). Copied from log1pf_2u1.c, with no
     special-case handling. See that file for details of the algorithm.  */
  float32x4_t m = vaddq_f32 (x, v_f32 (1.0f));
  uint32x4_t k
      = vandq_u32 (vsubq_u32 (vreinterpretq_u32_f32 (m), d->three_quarters),
		   v_u32 (0xff800000));
  float32x4_t s = vreinterpretq_f32_u32 (vsubq_u32 (d->four, k));
  float32x4_t m_scale
      = vreinterpretq_f32_u32 (vsubq_u32 (vreinterpretq_u32_f32 (x), k));
  m_scale = vaddq_f32 (m_scale, vfmaq_f32 (v_f32 (-1.0f), v_f32 (0.25f), s));
  float32x4_t p = eval_poly (m_scale, d->poly);
  float32x4_t scale_back = vmulq_f32 (vcvtq_f32_u32 (k), v_f32 (0x1.0p-23f));
  return vfmaq_f32 (p, scale_back, d->ln2);
}

#endif //  PL_MATH_V_LOG1PF_INLINE_H
