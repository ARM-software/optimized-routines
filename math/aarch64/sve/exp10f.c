/*
 * Single-precision SVE 10^x function.
 *
 * Copyright (c) 2023-2026, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#define _GNU_SOURCE
#include "sv_math.h"
#include "test_sig.h"
#include "test_defs.h"

/* For x < -SpecialBound, the result is subnormal and not handled
   correctly by FEXPA.  */
#define SpecialBound 0x1.2f702p+5 /* log10(2^126) ~ 37.93.  */

/* Values of x over which exp overflows or underflows.  */
#define InfBound 0x1.34ccccccccccdp+5 /* 38.6.  */
#define ZeroBound -0x1.68ccccccccccdp+5 /* -45.1.  */

static const struct data
{
  float log10_2, log2_10_hi, log2_10_lo, c1;
  float c0, shift, special_bound, inf_bound, zero_bound;
} data = {
  /* Coefficients generated using Remez algorithm with minimisation of relative
     error.  */
  .c0 = 0x1.26bb62p1,
  .c1 = 0x1.53524cp1,
  /* 1.5*2^17 + 127, a shift value suitable for FEXPA.  */
  .shift = 0x1.803f8p17f,
  .log10_2 = 0x1.a934fp+1,
  .log2_10_hi = 0x1.344136p-2,
  .log2_10_lo = -0x1.ec10cp-27,
  .special_bound = SpecialBound,
  .inf_bound = InfBound,
  .zero_bound = ZeroBound,
};

static inline svfloat32_t
sv_exp10f_inline (svfloat32_t x, const svbool_t pg, const struct data *d)
{
  /* exp10(x) = 2^(n/N) * 10^r = 2^n * (1 + poly (r)),
     with poly(r) in [1/sqrt(2), sqrt(2)] and
     x = r + n * log10(2) / N, with r in [-log10(2)/2N, log10(2)/2N].  */
  svfloat32_t lane_consts = svld1rq (svptrue_b32 (), &d->log10_2);

  /* n = round(x/(log10(2)/N)).  */
  svfloat32_t shift = sv_f32 (d->shift);
  svfloat32_t z = svmla_lane (shift, x, lane_consts, 0);
  svfloat32_t n = svsub_x (pg, z, shift);

  /* r = x - n*log10(2)/N.  */
  svfloat32_t r = x;
  r = svmls_lane (r, n, lane_consts, 1);
  r = svmls_lane (r, n, lane_consts, 2);

  svfloat32_t scale = svexpa (svreinterpret_u32 (z));

  /* Polynomial evaluation: poly(r) ~ exp10(r)-1.  */
  svfloat32_t poly = svmla_lane (sv_f32 (d->c0), r, lane_consts, 3);
  poly = svmul_x (pg, poly, r);

  return svmla_x (pg, scale, scale, poly);
}

static svfloat32_t NOINLINE
special_case (svfloat32_t x, svbool_t pg, svbool_t special,
	      const struct data *d)
{
  /* This deals with overflow and underflow in exponential for special case
     lanes.  */
  svbool_t is_inf = svcmpgt (pg, x, d->inf_bound);
  svbool_t is_zero = svcmplt (pg, x, d->zero_bound);

  /* The input `x` is further reduced (to `x/2`) to allow for accurate
     approximation on the interval `x > SpecialBound ~ 0x1.2f702p+5`.  */
  x = svmul_m (special, x, 0.5);

  /* Computes exp(x/2), and set lanes with underflow/overflow.  */
  svfloat32_t half_exp = sv_exp10f_inline (x, svptrue_b32 (), d);
  half_exp = svmul_m (special, half_exp, half_exp);
  half_exp = svsel (is_inf, sv_f32 (INFINITY), half_exp);

  return svsel (is_zero, sv_f32 (0), half_exp);
}

/* Single-precision SVE exp10f routine. Based on the FEXPA instruction.
   Worst case error is 2.86 ULP +0.50 ULP.
   _ZGVsMxv_exp10f (0x1.31b778p+5) got 0x1.ed399p+126
				  want 0x1.ed398ap+126.  */
svfloat32_t SV_NAME_F1 (exp10) (svfloat32_t x, const svbool_t pg)
{
  const struct data *d = ptr_barrier (&data);
  svbool_t special = svacgt (pg, x, d->special_bound);
  if (unlikely (svptest_any (special, special)))
    return special_case (x, pg, special, d);
  return sv_exp10f_inline (x, svptrue_b32 (), d);
}

#if WANT_EXP10_TESTS
TEST_SIG (SV, F, 1, exp10, -9.9, 9.9)
TEST_ULP (SV_NAME_F1 (exp10), 2.86)
/* Positive x.  */
TEST_INTERVAL (SV_NAME_F1 (exp10), 0, SpecialBound, 50000)
TEST_INTERVAL (SV_NAME_F1 (exp10), SpecialBound, InfBound, 50000)
TEST_INTERVAL (SV_NAME_F1 (exp10), InfBound, inf, 50000)
/* Negative x.  */
TEST_INTERVAL (SV_NAME_F1 (exp10), -0, ZeroBound, 50000)
TEST_INTERVAL (SV_NAME_F1 (exp10), ZeroBound, -inf, 50000)
/* Full range including NaNs.  */
TEST_INTERVAL (SV_NAME_F1 (exp10), 0, 0xffff0000, 50000)
#endif
CLOSE_SVE_ATTR
