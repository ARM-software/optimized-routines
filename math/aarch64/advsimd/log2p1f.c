/*
 * Single-precision vector log2(1+x) function.
 *
 * Copyright (c) 2025, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "test_defs.h"
#include "v_math.h"

static const struct data
{
  uint32x4_t four;
  int32x4_t three_quarters;
  float32x4_t c2, c4, c6, c8, c10, c12;
  float c1, c3, c5, c7, c9, c11, one_quarter, small;
  float32x4_t pinf, minf, nan;
} data = {
  /* Polynomial generated using FPMinimax in [-0.25, 0.5].  */
  .c1 = 0x1.715476p0,	   .c2 = V4 (-0x1.71548p-1),
  .c3 = 0x1.ec718p-2,	   .c4 = V4 (-0x1.714fecp-2),
  .c5 = 0x1.27498ep-2,	   .c6 = V4 (-0x1.ecd864p-3),
  .c7 = 0x1.ace5b4p-3,	   .c8 = V4 (-0x1.7800fcp-3),
  .c9 = 0x1.226c92p-3,	   .c10 = V4 (-0x1.92cbb2p-4),
  .c11 = 0x1.624cb2p-4,	   .c12 = V4 (-0x1.bb0f1p-5),
  .four = V4 (0x40800000), .three_quarters = V4 (0x3f400000),
  .one_quarter = 0.25f,	   .small = 0x1p-23f,
  .pinf = V4 (INFINITY),   .minf = V4 (-INFINITY),
  .nan = V4 (NAN)
};

static inline float32x4_t VPCS_ATTR
special_case (float32x4_t x, float32x4_t y, uint32x4_t cmp,
	      const struct data *d)
{
  y = vbslq_f32 (cmp, d->nan, y);
  uint32x4_t ret_pinf = vceqq_f32 (x, d->pinf);
  uint32x4_t ret_minf = vceqq_f32 (x, v_f32 (-1.0));
  y = vbslq_f32 (ret_pinf, d->pinf, y);
  return vbslq_f32 (ret_minf, d->minf, y);
}

/* Vector log2p1f approximation using polynomial on reduced interval.
   Worst-case error is 1.93 ULP:
   _ZGVnN4v_log2p1f(0x1.8789fcp-2) got 0x1.de58d4p-2
				  want 0x1.de58d8p-2.  */
float32x4_t VPCS_ATTR V_NAME_F1 (log2p1) (float32x4_t x)
{
  const struct data *d = ptr_barrier (&data);

  /* With x + 1 = t * 2^k (where t = m + 1 and k is chosen such that m
      is in [-0.25, 0.5]):
    log2p1(x) = log2(t) + log(2^k) = log2p1(m) + k.

    We approximate log2p1(m) with a polynomial, then scale by
    k. Instead of doing this directly, we use an intermediate
    scale factor s = 4*k to ensure the scale is representable
    as a normalised fp32 number.  */

  float32x4_t m = vaddq_f32 (x, v_f32 (1.0f));
  /* Choose k to scale x to the range [-1/4, 1/2].  */

  int32x4_t k
      = vandq_s32 (vsubq_s32 (vreinterpretq_s32_f32 (m), d->three_quarters),
		   vreinterpretq_s32_f32 (d->minf));
  uint32x4_t ku = vreinterpretq_u32_s32 (k);

  /* Scale up to ensure that the scale factor is representable as normalised
     fp32 number, and scale m down accordingly.  */
  float32x4_t s = vreinterpretq_f32_u32 (vsubq_u32 (d->four, ku));

  /* Scale x by exponent manipulation.  */
  float32x4_t m_scale
      = vreinterpretq_f32_u32 (vsubq_u32 (vreinterpretq_u32_f32 (x), ku));
  float32x4_t consts = vld1q_f32 (&d->c9);
  m_scale = vaddq_f32 (m_scale, vfmaq_laneq_f32 (v_f32 (-1.0f), s, consts, 2));

  float32x4_t scale_back = vmulq_laneq_f32 (vcvtq_f32_s32 (k), consts, 3);
  float32x4_t m2 = vmulq_f32 (m_scale, m_scale);

  /* Order-12 pairwise Horner.  */
  float32x4_t c1357 = vld1q_f32 (&d->c1);
  float32x4_t p23 = vfmaq_laneq_f32 (d->c2, m_scale, c1357, 1);
  float32x4_t p45 = vfmaq_laneq_f32 (d->c4, m_scale, c1357, 2);
  float32x4_t p67 = vfmaq_laneq_f32 (d->c6, m_scale, c1357, 3);
  float32x4_t p89 = vfmaq_laneq_f32 (d->c8, m_scale, consts, 0);
  float32x4_t p1011 = vfmaq_laneq_f32 (d->c10, m_scale, consts, 1);

  float32x4_t p = vfmaq_f32 (p1011, m2, d->c12);
  p = vfmaq_f32 (p89, m2, p);
  p = vfmaq_f32 (p67, m2, p);
  p = vfmaq_f32 (p45, m2, p);
  p = vfmaq_f32 (p23, m2, p);
  float32x4_t scaled_c1 = vfmaq_laneq_f32 (scale_back, m_scale, c1357, 0);
  uint32x4_t special_cases = vorrq_u32 (vmvnq_u32 (vcaltq_f32 (x, d->pinf)),
					vcleq_f32 (x, v_f32 (-1.0)));
  if (unlikely (v_any_u32 (special_cases)))
    return special_case (x, vfmaq_f32 (scaled_c1, m2, p), special_cases, d);

  return vfmaq_f32 (scaled_c1, m2, p);
}
HALF_WIDTH_ALIAS_F1 (log2p1)

#if WANT_C23_TESTS
TEST_DISABLE_FENV (V_NAME_F1 (log2p1))
TEST_ULP (V_NAME_F1 (log2p1), 1.43)
TEST_SYM_INTERVAL (V_NAME_F1 (log2p1), 0.0, 0x1p-23, 30000)
TEST_SYM_INTERVAL (V_NAME_F1 (log2p1), 0x1p-23, 1, 50000)
TEST_INTERVAL (V_NAME_F1 (log2p1), 1, inf, 50000)
TEST_INTERVAL (V_NAME_F1 (log2p1), -1.0, -inf, 1000)
#endif
