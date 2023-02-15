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
#include "math_config.h"

#define Four 0x40800000
#define Ln2 v_f32 (0x1.62e43p-1f)

#define C(i) v_f32 (__log1pf_data.coeffs[i])

static inline float32x4_t
eval_poly (float32x4_t m)
{
  /* Approximate log(1+m) on [-0.25, 0.5] using Estrin scheme.  */
  float32x4_t p_12 = vfmaq_f32 (C (0), m, C (1));
  float32x4_t p_34 = vfmaq_f32 (C (2), m, C (3));
  float32x4_t p_56 = vfmaq_f32 (C (4), m, C (5));
  float32x4_t p_78 = vfmaq_f32 (C (6), m, C (7));

  float32x4_t m2 = m * m;
  float32x4_t p_02 = vfmaq_f32 (m, m2, p_12);
  float32x4_t p_36 = vfmaq_f32 (p_34, m2, p_56);
  float32x4_t p_79 = vfmaq_f32 (p_78, m2, C (8));

  float32x4_t m4 = m2 * m2;
  float32x4_t p_06 = vfmaq_f32 (p_02, m4, p_36);

  return vfmaq_f32 (p_06, m4, m4 * p_79);
}

static inline float32x4_t
log1pf_inline (float32x4_t x)
{
  /* Helper for calculating log(x + 1). Copied from log1pf_2u1.c, with no
     special-case handling. See that file for details of the algorithm.  */
  float32x4_t m = x + 1.0f;
  uint32x4_t k = (v_as_u32_f32 (m) - 0x3f400000) & 0xff800000;
  float32x4_t s = v_as_f32_u32 (v_u32 (Four) - k);
  float32x4_t m_scale = v_as_f32_u32 (v_as_u32_f32 (x) - k)
			+ vfmaq_f32 (v_f32 (-1.0f), v_f32 (0.25f), s);
  float32x4_t p = eval_poly (m_scale);
  float32x4_t scale_back = vcvtq_f32_u32 (k) * 0x1.0p-23f;
  return vfmaq_f32 (p, scale_back, Ln2);
}

#endif //  PL_MATH_V_LOG1PF_INLINE_H
