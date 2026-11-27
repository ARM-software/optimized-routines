/*
 * Double-precision SVE 10^x function.
 *
 * Copyright (c) 2023-2026, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "sv_math.h"
#include "test_sig.h"
#include "test_defs.h"

/* Value of |x| above which scale overflows without special treatment.
   log10(2^1022) ~ 307.65.  */
#define SpecialBound 0x1.33a7146f72a41p+8

/* Values of x over which exp10 overflows or underflows.  */
#define InfBound 0x1.35p+8		/* 309.0.  */
#define ZeroBound -0x1.439b746e36b53p+8 /* ~ -323.61.  */

static const struct data
{
  double log2_10_hi, log2_10_lo;
  double log10_2, c0;
  double c1, c3, c2, c4;
  double shift, special_bound, inf_bound, zero_bound;
} data = {
  /* Coefficients generated using Remez algorithm.
     rel error: 0x1.9fcb9b3p-60
     abs error: 0x1.a20d9598p-60 in [ -log10(2)/128, log10(2)/128 ]
     max ulp err 0.52 +0.5.  */
  .c0 = 0x1.26bb1bbb55516p1,
  .c1 = 0x1.53524c73cd32ap1,
  .c2 = 0x1.0470591daeafbp1,
  .c3 = 0x1.2bd77b1361ef6p0,
  .c4 = 0x1.142b5d54e9621p-1,
  /* 1.5*2^46+1023. This value is further explained below.  */
  .shift = 0x1.800000000ffc0p+46,
  .log10_2 = 0x1.a934f0979a371p1,     /* 1/log2(10).  */
  .log2_10_hi = 0x1.34413509f79ffp-2, /* log2(10).  */
  .log2_10_lo = -0x1.9dc1da994fd21p-59,
  .special_bound = SpecialBound,
  .inf_bound = InfBound,
  .zero_bound = ZeroBound,
};

static inline svfloat64_t
exp10_inline (svfloat64_t x, const svbool_t pg, const struct data *d)
{
  /* n is x/log10(2) rounded to the nearest multiple of 1/64.  */
  svfloat64_t shift = sv_f64 (d->shift);
  svfloat64_t log10_2_c0 = svld1rq (svptrue_b64 (), &d->log10_2);
  svfloat64_t z = svmla_lane (shift, x, log10_2_c0, 0);
  svfloat64_t n = svsub_x (pg, z, shift);

  /* r = x - n*log10(2).  */
  svfloat64_t log2_10 = svld1rq (svptrue_b64 (), &d->log2_10_hi);
  svfloat64_t r = x;
  r = svmls_lane (r, n, log2_10, 0);
  r = svmls_lane (r, n, log2_10, 1);

  svfloat64_t scale = svexpa (svreinterpret_u64 (z));
  svfloat64_t c24 = svld1rq (svptrue_b64 (), &d->c2);

  /* Approximate exp10(r) using polynomial.  */
  svfloat64_t r2 = svmul_x (svptrue_b64 (), r, r);
  svfloat64_t p12 = svmla_lane (sv_f64 (d->c1), r, c24, 0);
  svfloat64_t p34 = svmla_lane (sv_f64 (d->c3), r, c24, 1);
  svfloat64_t p14 = svmla_x (pg, p12, p34, r2);

  svfloat64_t poly = svmla_x (pg, svmul_lane (r, log10_2_c0, 1), r2, p14);

  return svmla_x (pg, scale, scale, poly);
}

static svfloat64_t NOINLINE
special_case (svfloat64_t x, const svbool_t pg, const struct data *d)
{
  /* Computes the offset and scale factor based on sign of the input.  */
  svbool_t is_negative = svcmplt (pg, x, 0.0);
  svfloat64_t offset = svneg_m (sv_f64 (53.0), is_negative, sv_f64 (53.0));
  svint64_t scale_adjust = svneg_m (sv_s64 (53), is_negative, sv_s64 (53));

  /* Bounds x between zero_bound and inf_bound, where exp10(x) would return
     0 or inf. By clamping x to these bounds, the behaviour of large values
     is more predictable.  */
  x = svmin_x (pg, svmax_x (pg, x, d->zero_bound), d->inf_bound);

  /* exp10(x) = 2^(n/N) * 10^r = 2^n * (1 + poly(r)),
     with 1 + poly(r) in [1/sqrt(2), sqrt(2)] and
     x = r + n * log10(2) / N, with r in [-log10(2)/2N, log10(2)/2N].  */
  svfloat64_t shift = sv_f64 (d->shift);
  svfloat64_t log10_2_c0 = svld1rq (svptrue_b64 (), &d->log10_2);

  /* n is x/log10(2) rounded to the nearest multiple of 1/64.  */
  svfloat64_t z = svmla_lane (shift, x, log10_2_c0, 0);
  svfloat64_t n = svsub_x (pg, z, shift);

  /* r = x - n*log10(2).  */
  svfloat64_t log2_10 = svld1rq (svptrue_b64 (), &d->log2_10_hi);
  svfloat64_t r = x;
  r = svmls_lane (r, n, log2_10, 0);
  r = svmls_lane (r, n, log2_10, 1);

  /* Computes scale with an offset, which returns scale = 2^(n - 53).  */
  z = svsub_x (pg, z, offset);
  svfloat64_t scale = svexpa (svreinterpret_u64 (z));

  svfloat64_t c24 = svld1rq (svptrue_b64 (), &d->c2);

  /* Approximate exp10(r) using polynomial.  */
  svfloat64_t r2 = svmul_x (svptrue_b64 (), r, r);
  svfloat64_t p12 = svmla_lane (sv_f64 (d->c1), r, c24, 0);
  svfloat64_t p34 = svmla_lane (sv_f64 (d->c3), r, c24, 1);
  svfloat64_t p14 = svmla_x (pg, p12, p34, r2);

  svfloat64_t poly = svmla_x (pg, svmul_lane (r, log10_2_c0, 1), r2, p14);

  /* Reconstruct y as 2^(n - 53) * (1 + poly(r)).  */
  svfloat64_t y = svmla_x (pg, scale, scale, poly);

  /* Scale the result by 2^53:
     2^n * (1 + poly(r)) = 2^53 * 2^(n - 53) * (1 + poly(r)).  */
  return svscale_x (pg, y, scale_adjust);
}

/* Vector version of exp10
   The maximum observed error is 0.52 + 0.5 ULP.
   _ZGVsMxv_exp10(-0x1.2862fec805e58p+2)
    got 0x1.885a89551d782p-16
   want 0x1.885a89551d781p-16.  */
svfloat64_t SV_NAME_D1 (exp10) (svfloat64_t x, svbool_t pg)
{
  const struct data *d = ptr_barrier (&data);
  svbool_t special = svacgt (pg, x, d->special_bound);
  if (unlikely (svptest_any (special, special)))
    return special_case (x, pg, d);
  return exp10_inline (x, pg, d);
}

#if WANT_EXP10_TESTS
TEST_SIG (SV, D, 1, exp10, -9.9, 9.9)
TEST_ULP (SV_NAME_D1 (exp10), 0.52)
TEST_SYM_INTERVAL (SV_NAME_D1 (exp10), 0, SpecialBound, 10000)
TEST_INTERVAL (SV_NAME_D1 (exp10), SpecialBound, InfBound, 10000)
TEST_INTERVAL (SV_NAME_D1 (exp10), -SpecialBound, ZeroBound, 10000)
TEST_INTERVAL (SV_NAME_D1 (exp10), InfBound, inf, 1000)
TEST_INTERVAL (SV_NAME_D1 (exp10), ZeroBound, -inf, 1000)
#endif
CLOSE_SVE_ATTR
