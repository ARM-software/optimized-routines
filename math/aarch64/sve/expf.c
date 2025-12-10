/*
 * Single-precision vector e^x function.
 *
 * Copyright (c) 2019-2025, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "sv_math.h"
#include "test_sig.h"
#include "test_defs.h"

/* Roughly 87.3. For x < -SpecialBound, the result is subnormal and not handled
   correctly by FEXPA.  */
#define SpecialBound 0x1.5d5e2ap+6f

/* Value of n which exp overflows or underflows.  */
#define InfBound 0x1.62e42fp6f
#define ZeroBound -0x1.9fe368p6f

static const struct data
{
  float ln2_hi, ln2_lo, c1, c3, c2;
  float inv_ln2, shift;
  float32_t special_bound, inf_bound, zero_bound;
} data = {
  /* shift = 1.5*2^17 + 127.  */
  .shift = 0x1.803f8p17f,
  .inv_ln2 = 0x1.715476p+0f,
  .ln2_hi = 0x1.62e4p-1f,
  .ln2_lo = 0x1.7f7d1cp-20f,
  /* Coefficients generated using Remez algorithm with minimisation of relative
     error.  */
  .c1 = 0.5f,
  .c2 = 0x1.55559p-3,
  .c3 = 0x1.55555cp-5,
  .special_bound = SpecialBound,
  .inf_bound = InfBound,
  .zero_bound = ZeroBound,
};

static inline svfloat32_t
expf_inline (svfloat32_t x, const svbool_t pg, const struct data *d)
{
  /* exp(x) = 2^n (1 + poly(r)), with 1 + poly(r) in [1/sqrt(2),sqrt(2)]
    x = ln2*n + r, with r in [-ln2/2, ln2/2].  */
  svfloat32_t lane_constants = svld1rq (svptrue_b32 (), &d->ln2_hi);

  /* n = round(x/(ln2/N)).  */
  svfloat32_t z = svmad_x (pg, sv_f32 (d->inv_ln2), x, d->shift);
  svfloat32_t n = svsub_x (pg, z, d->shift);

  /* r = x - n*ln2/N.  */
  svfloat32_t r = x;
  r = svmls_lane (r, n, lane_constants, 0);
  r = svmls_lane (r, n, lane_constants, 1);

  /* scale = 2^(n/N).  */
  svfloat32_t scale = svexpa (svreinterpret_u32 (z));

  /* poly(r) = exp(r) - 1 ~= r + 0.5 r^2.  */
  svfloat32_t r2 = svmul_x (svptrue_b32 (), r, r);
  svfloat32_t poly = svmla_lane (r, r2, lane_constants, 2);

  return svmla_x (pg, scale, scale, poly);
}

/* This is the version used if there is any special lane in the input vector.
   The approximation needs to match that of the fast path.
   To achieve this we assemble the same polynomial, ie `r + 0.5 * r^2`,
   then we conditionally add an extra `c2 * r^3` term.  */
static inline svfloat32_t
expf_slow_inline (svfloat32_t x, const svbool_t special, const struct data *d)
{
  svfloat32_t lane_constants = svld1rq (svptrue_b32 (), &d->ln2_hi);

  svfloat32_t z = svmad_x (svptrue_b32 (), sv_f32 (d->inv_ln2), x, d->shift);
  svfloat32_t n = svsub_x (svptrue_b32 (), z, d->shift);

  svfloat32_t r = x;
  r = svmls_lane (r, n, lane_constants, 0);
  r = svmls_lane (r, n, lane_constants, 1);

  svfloat32_t scale = svexpa (svreinterpret_u32 (z));

  /* poly(r) = exp(r) - 1 ~= r + 0.5 r^2 + c2 r^3.  */
  svfloat32_t r2 = svmul_x (svptrue_b32 (), r, r);
  svfloat32_t r3 = svmul_x (svptrue_b32 (), r, r2);
  svfloat32_t poly_slow = svmla_lane (sv_f32 (d->c2), r, lane_constants, 3);
  svfloat32_t poly_fast = svmla_lane (r, r2, lane_constants, 2);
  svfloat32_t poly = svmla_x (special, poly_fast, poly_slow, r3);

  return svmla_x (svptrue_b32 (), scale, scale, poly);
}

static svfloat32_t NOINLINE
special_case (svfloat32_t x, svbool_t pg, svbool_t special,
	      const struct data *d)
{
  /* This deals with overflow and underflow in exponential for special case
     lanes.  */
  svbool_t is_inf = svcmpgt (pg, x, d->inf_bound);
  svbool_t is_zero = svcmplt (pg, x, d->zero_bound);
  svfloat32_t limit = svsel (is_inf, sv_f32 (INFINITY), sv_f32 (0));
  svbool_t is_uoflow = svorr_b_z (pg, is_inf, is_zero);

  /* The input `x` is further reduced (to `x/2`) to allow for accurate
     approximation on the interval `x > SpecialBound ~ 87.3`.  */
  x = svmul_m (special, x, 0.5);

  /* Computes exp(x/2), and set lanes with underflow/overflow.  */
  svfloat32_t half_exp = expf_slow_inline (x, special, d);
  half_exp = svsel (is_uoflow, limit, half_exp);

  return svmul_m (special, half_exp, half_exp);
}

/* Optimised single-precision SVE exp function.
   Worst-case error is 2.70 +0.50 ULP:
   _ZGVsMxv_expf(0x1.5fec38p+6) got 0x1.e7831ep+126
			       want 0x1.e78318p+126.  */
svfloat32_t SV_NAME_F1 (exp) (svfloat32_t x, const svbool_t pg)
{
  const struct data *d = ptr_barrier (&data);
  svbool_t is_special_case = svacgt (pg, x, d->special_bound);
  if (unlikely (svptest_any (pg, is_special_case)))
    return special_case (x, pg, is_special_case, d);
  return expf_inline (x, pg, d);
}

TEST_SIG (SV, F, 1, exp, -9.9, 9.9)
TEST_ULP (SV_NAME_F1 (exp), 2.71)
/* Positive x.  */
TEST_INTERVAL (SV_NAME_F1 (exp), 0, SpecialBound, 50000)
TEST_INTERVAL (SV_NAME_F1 (exp), SpecialBound, InfBound, 50000)
TEST_INTERVAL (SV_NAME_F1 (exp), InfBound, inf, 50000)
/* Negative x.  */
TEST_INTERVAL (SV_NAME_F1 (exp), -0, ZeroBound, 50000)
TEST_INTERVAL (SV_NAME_F1 (exp), ZeroBound, -inf, 50000)
/* Full range including NaNs.  */
TEST_INTERVAL (SV_NAME_F1 (exp), 0, 0xffff0000, 50000)
CLOSE_SVE_ATTR
