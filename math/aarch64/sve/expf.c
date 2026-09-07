/*
 * Single-precision vector e^x function.
 *
 * Copyright (c) 2019-2026, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "sv_math.h"
#include "test_sig.h"
#include "test_defs.h"

/* For x < -SpecialBound, the result is subnormal and not handled
   correctly by FEXPA.  */
#define SpecialBound 0x1.5d589ep+6f /* ln(2^126) ~ 87.34.  */

/* Values of x which exp overflows or underflows.  */
#define InfBound 0x1.62e42fp6f /* ~ 88.72.  */
#define ZeroBound -0x1.9fe368p6f /* ~ 103.97.  */

static const struct data
{
  float ln2_hi, ln2_lo, c1;
  float shift, inv_ln2;
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

/* Special case to handle large magnitudes of x, where fexpa returns nan.
   By applying a fixed offset to the value passed to fexpa,
   and then reapplying a scaling factor afterwards, this is avoided.  */
static svfloat32_t NOINLINE
special_case (svfloat32_t x, const svbool_t pg, const struct data *d)
{
  /* Computes the offset and scale factor based on sign of the input.  */
  svbool_t is_negative = svcmplt (pg, x, 0.0f);
  svfloat32_t offset = svneg_m (sv_f32 (23.0f), is_negative, sv_f32 (23.0f));
  svint32_t scale_adjust = svneg_m (sv_s32 (23), is_negative, sv_s32 (23));

  /* Bounds x between zero_bound and inf_bound, where exp(x) would return
     0 or inf. By clamping x to these bounds, the behaviour of large values
     is more predictable.  */
  x = svmin_x (pg, svmax_x (pg, x, d->zero_bound), d->inf_bound);

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

  /* Computes scale with an offset, which returns:
     scale = 2^(n/N - 23).  */
  z = svsub_x (pg, z, offset);
  svfloat32_t scale = svexpa (svreinterpret_u32 (z));

  /* poly(r) = exp(r) - 1 ~= r + 0.5 r^2.  */
  svfloat32_t r2 = svmul_x (svptrue_b32 (), r, r);
  svfloat32_t poly = svmla_lane (r, r2, lane_constants, 2);

  /* Reconstruct y as 2^(n/N - 23) * (1 + poly(r)).  */
  svfloat32_t y = svmla_x (pg, scale, scale, poly);

  /* Scale the result by 2^23:
     2^(n/N) * (1 + poly(r)) = 2^23 * 2^(n/N - 23) * (1 + poly(r)).  */
  return svscale_x (pg, y, scale_adjust);
}

/* Vector version of expf.
   The maximum observed error is 0.88 + 0.5 ULP.
   _ZGVsMxv_expf (-0x1.bba276p-6)
    got 0x1.f25288p-1
   want 0x1.f2528ap-1.  */
svfloat32_t SV_NAME_F1 (exp) (svfloat32_t x, const svbool_t pg)
{
  const struct data *d = ptr_barrier (&data);
  svbool_t special = svacgt (pg, x, d->special_bound);
  if (unlikely (svptest_any (special, special)))
    return special_case (x, svptrue_b32 (), d);
  return expf_inline (x, svptrue_b32 (), d);
}

TEST_SIG (SV, F, 1, exp, -9.9, 9.9)
TEST_ULP (SV_NAME_F1 (exp), 0.89)
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
