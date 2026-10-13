/*
 * Correctly rounded half-precision vector e^x function.
 *
 * Copyright (c) 2026, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "mathlib.h"
#include "v_math.h"
#include "test_sig.h"
#include "test_defs.h"

/* For |x| >= 18, exp(x) rounds to zero or infinity in binary16.  */
#define SpecialBound 0x1.2p+4

static const struct data
{
  float32x4_t c2, c3, lane_consts;
  float16x8_t special_bound;
} data = {
  /* Degree-5 relative minimax polynomial.
     Generated with the one-shot relative-error Remez construction in
     math/tools/expf16.sollya, using deg = 5 and rounding coefficients to
     binary32.  */
  .c2 = V4 (0x1.fffdfcp-2f),
  .c3 = V4 (0x1.5557aep-3f),
  /* { inv_ln2, ln2, c4, c5 }.  */
  .lane_consts
  = { 0x1.715476p+0f, 0x1.62e43p-1f, 0x1.5729fp-5f, 0x1.106284p-7f },
  .special_bound = V8 (SpecialBound),
};

/* exp(x) = 2^n * exp(r), where x = n * ln2 + r.  Complete range reduction
   and polynomial evaluation in FP32.  */
static inline float32x4_t VPCS_ATTR
inline_expf16 (float32x4_t x, const struct data *d)
{
  const float32x4_t one = vdupq_n_f32 (1.0f);
  const float32x4_t lane_consts = d->lane_consts;

  float32x4_t n = vrndnq_f32 (vmulq_laneq_f32 (x, lane_consts, 0));
  int32x4_t ni = vcvtq_s32_f32 (n);
  float32x4_t r = vfmsq_laneq_f32 (x, n, lane_consts, 1);
  float32x4_t r2 = vmulq_f32 (r, r);
  float32x4_t even = vfmaq_laneq_f32 (d->c2, r2, lane_consts, 2);
  float32x4_t odd = vfmaq_laneq_f32 (d->c3, r2, lane_consts, 3);
  float32x4_t q = vfmaq_f32 (even, odd, r);
  float32x4_t f = vfmaq_f32 (vaddq_f32 (one, r), q, r2);
  int32x4_t bias = vdupq_n_s32 (127);
  int32x4_t e = vshlq_n_s32 (vaddq_s32 (ni, bias), 23);
  return vmulq_f32 (f, vreinterpretq_f32_s32 (e));
}

float16x8_t VPCS_ATTR FP16_ATTR NOINLINE V_NAME_H1 (exp) (float16x8_t x)
{
  const struct data *d = ptr_barrier (&data);

  float32x4_t y0 = inline_expf16 (vcvt_f32_f16 (vget_low_f16 (x)), d);
  float32x4_t y1 = inline_expf16 (vcvt_high_f32_f16 (x), d);
  float16x8_t y = vcvt_high_f16_f32 (vcvt_f16_f32 (y0), y1);

  uint16x8_t special = vcageq_f16 (x, d->special_bound);
  float16x8_t zero_or_inf
      = vbslq_f16 (vcltzq_f16 (x), vdupq_n_f16 (0), vdupq_n_f16 (INFINITY));
  y = vbslq_f16 (special, zero_or_inf, y);

  /* These two inputs are closer to a binary16 rounding boundary than the
     binary32 evaluation can distinguish.  Exhaustive MPFR comparison shows
     that both results must be decremented by one ULP.  */
  uint16x8_t ix = vreinterpretq_u16_f16 (x);
  uint16x8_t hard = vorrq_u16 (vceqq_u16 (ix, vdupq_n_u16 (0x1f79)),
			       vceqq_u16 (ix, vdupq_n_u16 (0x25cf)));
  uint16x8_t iy = vreinterpretq_u16_f16 (y);
  iy = vsubq_u16 (iy, vandq_u16 (hard, vdupq_n_u16 (1)));
  return vreinterpretq_f16_u16 (iy);
}

HALF_WIDTH_ALIAS_H1 (exp)

TEST_SIG (V, H, 1, exp, -9.9, 9.9)
TEST_ULP (V_NAME_H1 (exp), 0.00)
TEST_SYM_INTERVAL (V_NAME_H1 (exp), 0, 0x1p-10, 5000)
TEST_SYM_INTERVAL (V_NAME_H1 (exp), 0x1p-10, 0.5, 10000)
TEST_SYM_INTERVAL (V_NAME_H1 (exp), 0.5, SpecialBound, 20000)
TEST_SYM_INTERVAL (V_NAME_H1 (exp), SpecialBound, 0x1.8p4, 20000)
TEST_SYM_INTERVAL (V_NAME_H1 (exp), 0x1.8p4, inf, 10000)
