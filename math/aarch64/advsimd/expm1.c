/*
 * Double-precision vector exp(x) - 1 function.
 *
 * Copyright (c) 2022-2026, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "v_math.h"
#include "test_sig.h"
#include "test_defs.h"

/* Value of |x| above which scale overflows without special treatment.  */
#define SpecialBound 0x1.62b7d369a5aa9p+9 /* ~709.43.  */

/* Value of n above which scale overflows even with special treatment.  */
#define ScaleBound 0x1.4p+10 /* 1280.0.  */

static const struct data
{
  float64x2_t c2, c4, c6, c8;
  float64x2_t invln2;
  int64x2_t exponent_bias;
  uint64x2_t special_offset, special_bias, special_bias2;
  double c1, c3, c5, c7, c9, c10;
  double ln2[2];
  float64x2_t special_bound, scale_thresh;
} data = {
  .c1 = 0x1.5555555555559p-3,
  .c2 = V2 (0x1.555555555554bp-5),
  .c3 = 0x1.111111110f663p-7,
  .c4 = V2 (0x1.6c16c16c1b5f3p-10),
  .c5 = 0x1.a01a01affa35dp-13,
  .c6 = V2 (0x1.a01a018b4ecbbp-16),
  .c7 = 0x1.71ddf82db5bb4p-19,
  .c8 = V2 (0x1.27e517fc0d54bp-22),
  .c9 = 0x1.af5eedae67435p-26,
  .c10 = 0x1.1f143d060a28ap-29,
  .ln2 = { 0x1.62e42fefa39efp-1, 0x1.abc9e3b39803fp-56 },
  .invln2 = V2 (0x1.71547652b82fep0),
  .exponent_bias = V2 (0x3ff0000000000000),
  .special_offset = V2 (0x6000000000000000), /* 0x1p513.  */
  .special_bias = V2 (0x7000000000000000),   /* 0x1p769.  */
  .special_bias2 = V2 (0x3010000000000000),  /* 0x1p-254.  */
  /* Value above which expm1(x) should overflow. Absolute value of the
     underflow bound is greater than this, so it catches both cases - there is
     a small window where fallbacks are triggered unnecessarily.  */
  .scale_thresh = V2 (ScaleBound),
  .special_bound = V2 (SpecialBound),
};

static inline float64x2_t VPCS_ATTR
special_case (float64x2_t poly, float64x2_t n, float64x2_t scale,
	      const struct data *d)
{
  /* 2^n may overflow, break it up into s1*s2.  */
  uint64x2_t b = vandq_u64 (vclezq_f64 (n), d->special_offset);
  float64x2_t s1 = vreinterpretq_f64_u64 (vsubq_u64 (d->special_bias, b));
  float64x2_t s2 = vreinterpretq_f64_u64 (vaddq_u64 (
      vsubq_u64 (vreinterpretq_u64_f64 (scale), d->special_bias2), b));
  uint64x2_t cmp2 = vcagtq_f64 (n, d->scale_thresh);
  float64x2_t r1 = vmulq_f64 (s1, s1);
  float64x2_t r2 = vmulq_f64 (vfmaq_f64 (s2, poly, s2), s1);
  /* Similar to r1 but avoids double rounding in the subnormal range.  */
  return vsubq_f64 (vbslq_f64 (cmp2, r1, r2), v_f64 (1.0f));
}

/* Double-precision vector exp(x) - 1 function.
   The maximum error observed error is 1.55 +0.5 ULP:
  _ZGVnN2v_expm1(0x1.6329669eb8c87p-2) got 0x1.a8897eef87b34p-2
				      want 0x1.a8897eef87b32p-2.  */
float64x2_t VPCS_ATTR V_NAME_D1 (expm1) (float64x2_t x)
{
  const struct data *d = ptr_barrier (&data);

  /* Helper routine for calculating exp(x) - 1.  */

  float64x2_t ln2 = vld1q_f64 (&d->ln2[0]);

  /* Reduce argument to smaller range:
     Let i = round(x / ln2)
     and f = x - i * ln2, then f is in [-ln2/2, ln2/2].
     exp(x) - 1 = 2^i * (expm1(f) + 1) - 1
     where 2^i is exact because i is an integer.  */
  float64x2_t n = vrndaq_f64 (vmulq_f64 (x, d->invln2));
  int64x2_t i = vcvtq_s64_f64 (n);
  float64x2_t f = vfmsq_laneq_f64 (x, n, ln2, 0);
  f = vfmsq_laneq_f64 (f, n, ln2, 1);

  /* Approximate expm1(f) using polynomial.
     Taylor expansion for expm1(x) has the form:
	 x + ax^2 + bx^3 + cx^4 ....
     So we calculate the polynomial P(f) = a + bf + cf^2 + ...
     and assemble the approximation expm1(f) ~= f + f^2 * P(f).  */
  float64x2_t f2 = vmulq_f64 (f, f);
  float64x2_t f4 = vmulq_f64 (f2, f2);
  float64x2_t lane_consts_13 = vld1q_f64 (&d->c1);
  float64x2_t lane_consts_57 = vld1q_f64 (&d->c5);
  float64x2_t lane_consts_910 = vld1q_f64 (&d->c9);
  float64x2_t p01 = vfmaq_laneq_f64 (v_f64 (0.5), f, lane_consts_13, 0);
  float64x2_t p23 = vfmaq_laneq_f64 (d->c2, f, lane_consts_13, 1);
  float64x2_t p45 = vfmaq_laneq_f64 (d->c4, f, lane_consts_57, 0);
  float64x2_t p67 = vfmaq_laneq_f64 (d->c6, f, lane_consts_57, 1);
  float64x2_t p03 = vfmaq_f64 (p01, f2, p23);
  float64x2_t p47 = vfmaq_f64 (p45, f2, p67);
  float64x2_t p89 = vfmaq_laneq_f64 (d->c8, f, lane_consts_910, 0);
  float64x2_t p = vfmaq_laneq_f64 (p89, f2, lane_consts_910, 1);
  p = vfmaq_f64 (p47, f4, p);
  p = vfmaq_f64 (p03, f4, p);

  p = vfmaq_f64 (f, f2, p);

  /* Assemble the result.
     expm1(x) ~= 2^i * (p + 1) - 1
     Let scale = 2^i.  */
  int64x2_t u = vaddq_s64 (vshlq_n_s64 (i, 52), d->exponent_bias);
  float64x2_t scale = vreinterpretq_f64_s64 (u);

  float64x2_t y = vfmaq_f64 (vsubq_f64 (scale, v_f64 (1.0)), p, scale);

  uint64x2_t special = vcageq_f64 (x, d->special_bound);

  /* Fallback to special case for lanes with overflow.  */
  if (unlikely (v_any_u64 (special)))
    return vbslq_f64 (special, special_case (p, n, scale, d), y);

  /* expm1(x) ~= p * scale + (scale - 1).  */
  return y;
}

TEST_SIG (V, D, 1, expm1, -9.9, 9.9)
TEST_ULP (V_NAME_D1 (expm1), 1.56)
TEST_SYM_INTERVAL (V_NAME_D1 (expm1), 0, 0x1p-51, 10000)
TEST_SYM_INTERVAL (V_NAME_D1 (expm1), 0x1p-51, SpecialBound, 100000)
TEST_SYM_INTERVAL (V_NAME_D1 (expm1), SpecialBound, ScaleBound, 100000)
TEST_SYM_INTERVAL (V_NAME_D1 (expm1), ScaleBound, inf, 10000)