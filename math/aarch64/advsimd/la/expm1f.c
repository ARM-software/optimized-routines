/*
 * Single-precision vector exp(x) - 1 function.
 *
 * Copyright (c) 2022-2026, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "v_math.h"
#include "test_sig.h"
#include "test_defs.h"
#include "v_expf_special_inline.h"

/* Value of |x| above which scale overflows without special treatment.  */
#define SpecialBound 0x1.4d814bbe4473dp+6 /* ≈ 88.3763.  */

static const struct data
{
  struct v_expf_special_data special_data;
  uint32x4_t exponent_bias;
  float32x4_t c0, c2;
  float c1, c3, inv_ln2, c4;
  float ln2_hi, ln2_lo;
  float32x4_t special_bound;
} data = {
  .special_data = V_EXPF_SPECIAL_DATA,
  /* Coefficients generated using fpminimax with degree=5 in [-log(2)/2,
     log(2)/2]. Exponent bias is asuint(1.0f).  */
  .c0 = V4 (0x1.fffffep-2),
  .c1 = 0x1.5554aep-3,
  .c2 = V4 (0x1.555736p-5),
  .c3 = 0x1.12287cp-7,
  .c4 = 0x1.6b55a2p-10,
  .exponent_bias = V4 (0x3f800000),
  .inv_ln2 = 0x1.715476p+0f,
  .ln2_hi = 0x1.62e4p-1f,
  .ln2_lo = 0x1.7f7d1cp-20f,
  /* Implementation triggers special case handling as soon as the scale
     overflows, which is earlier than expm1f's overflow bound
     `log1p(FLT_MAX)~88.7` or underflow bound `log1p(FLT_MIN)~-103.28`.
     The absolute comparison catches all these cases efficiently, although
     there is a small window where special cases are triggered
     unnecessarily.  */
  .special_bound = V4 (SpecialBound),
};

/* Single-precision vector exp(x) - 1 function.
   The maximum error is 1.62 ULP:
   _ZGVnN4v_expm1f(0x1.85f83p-2) got 0x1.da9f4p-2
				want 0x1.da9f44p-2.  */
float32x4_t VPCS_ATTR NOINLINE V_NAME_F1 (expm1) (float32x4_t x)
{
  const struct data *d = ptr_barrier (&data);

  float32x4_t lane_consts = vld1q_f32 (&d->c1);

  /* exp(x) = 2^n * e^r = 2^n * (1 + poly (r)),
     with 1 + poly(r) in [1/sqrt(2), sqrt(2)] and
     x = r + n * ln(2), with r in [-ln(2)/2, ln(2)/2].  */
  float32x4_t n = vrndaq_f32 (vmulq_laneq_f32 (x, lane_consts, 2));
  float32x2_t ln2 = vld1_f32 (&d->ln2_hi);
  float32x4_t r = vfmsq_lane_f32 (x, n, ln2, 0);
  r = vfmsq_lane_f32 (r, n, ln2, 1);

  /* scale = 2^i.  */
  uint32x4_t e = vshlq_n_u32 (vreinterpretq_u32_s32 (vcvtaq_s32_f32 (n)), 23);
  float32x4_t scale = vreinterpretq_f32_u32 (vaddq_u32 (e, d->exponent_bias));

  /* Handles very large values (+ve and -ve), +/-NaN, +/-Inf.  */
  uint32x4_t cmp = vcageq_f32 (x, d->special_bound);

  /* Approximate expm1(r) with polynomial P, expm1(r) ~= r + r^2 * P(r).  */
  float32x4_t r2 = vmulq_f32 (r, r);
  float32x4_t r4 = vmulq_f32 (r2, r2);
  float32x4_t p01 = vfmaq_laneq_f32 (d->c0, r, lane_consts, 0);
  float32x4_t p23 = vfmaq_laneq_f32 (d->c2, r, lane_consts, 1);
  float32x4_t poly = vfmaq_f32 (p01, r2, p23);
  poly = vfmaq_laneq_f32 (poly, r4, lane_consts, 3);
  poly = vfmaq_f32 (r, r2, poly);

  /* expm1(x) ~= p * scale + (scale - 1).  */
  float32x4_t y = vfmaq_f32 (vsubq_f32 (scale, v_f32 (1.0f)), poly, scale);

  /* Fallback to special case for lanes with overflow.  */
  if (unlikely (v_any_u32 (cmp)))
    {
      float32x4_t special
	  = (expf_special (poly, n, e, cmp, scale, &d->special_data));
      float32x4_t specialm1 = vsubq_f32 (special, v_f32 (1.0f));
      return vbslq_f32 (cmp, specialm1, y);
    }
  return y;
}

HALF_WIDTH_ALIAS_F1 (expm1)

TEST_SIG (V, F, 1, expm1, -9.9, 9.9)
TEST_ULP (V_NAME_F1 (expm1), 1.13)
TEST_SYM_INTERVAL (V_NAME_F1 (expm1), 0, 0x1p-23, 10000)
TEST_SYM_INTERVAL (V_NAME_F1 (expm1), 0x1p-23, SpecialBound, 1000000)
TEST_SYM_INTERVAL (V_NAME_F1 (expm1), SpecialBound, 0x1.8p+7, 1000000)
TEST_SYM_INTERVAL (V_NAME_F1 (expm1), 0x1.8p+7, inf, 10000)
