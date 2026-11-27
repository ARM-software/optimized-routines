/*
 * Double-precision SVE 2^x function.
 *
 * Copyright (c) 2023-2026, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "sv_math.h"
#include "test_sig.h"
#include "test_defs.h"

/* Value of |x| above which scale overflows without special treatment.
   log2(2^1022) = 1022.00.  */
#define SpecialBound 0x1.ffp+9

/* Values of x which exp2 overflows or underflows.  */
#define InfBound 0x1p10		 /* 1024.  */
#define ZeroBound -0x1.0d008p+10 /* -1076.00781253.  */

static const struct data
{
  double c2, c4;
  double c0, c1, c3;
  double shift, special_bound;
  double inf_bound, zero_bound;
} data = {
  /* Coefficients are computed using Remez algorithm with
     minimisation of the absolute error.  */
  .c0 = 0x1.62e42fefa39efp-1,	 .c1 = 0x1.ebfbdff82a31bp-3,
  .c2 = 0x1.c6b08d706c8a5p-5,	 .c3 = 0x1.3b2ad2ff7d2f3p-7,
  .c4 = 0x1.5d8761184beb3p-10,	 .shift = 0x1.800000000ffc0p+46,
  .special_bound = SpecialBound, .inf_bound = InfBound,
  .zero_bound = ZeroBound,
};

static svfloat64_t inline exp2_inline (svfloat64_t x, svbool_t pg,
				       const struct data *d)
{
  svfloat64_t z = svadd_x (svptrue_b64 (), x, d->shift);
  svfloat64_t n = svsub_x (pg, z, d->shift);
  svfloat64_t r = svsub_x (pg, x, n);

  svfloat64_t scale = svexpa (svreinterpret_u64 (z));

  svfloat64_t r2 = svmul_x (svptrue_b64 (), r, r);
  svfloat64_t c24 = svld1rq (svptrue_b64 (), &d->c2);

  /* Approximate exp2(r) using polynomial.  */
  /* y = exp2(r) - 1 ~= r * (C0 + C1 r + C2 r^2 + C3 r^3 + C4 r^4).  */
  svfloat64_t p12 = svmla_lane (sv_f64 (d->c1), r, c24, 0);
  svfloat64_t p34 = svmla_lane (sv_f64 (d->c3), r, c24, 1);
  svfloat64_t p = svmla_x (pg, p12, p34, r2);
  p = svmad_x (pg, p, r, d->c0);
  svfloat64_t poly = svmul_x (svptrue_b64 (), r, p);

  return svmla_x (pg, scale, scale, poly);
}

static svfloat64_t NOINLINE
special_case (svfloat64_t x, svbool_t pg, const struct data *d)
{
  /* Computes the offset and scale factor based on sign of the input.  */
  svbool_t is_negative = svcmplt (pg, x, 0.0);
  svfloat64_t offset = svneg_m (sv_f64 (53.0), is_negative, sv_f64 (53.0));
  svint64_t scale_adjust = svneg_m (sv_s64 (53), is_negative, sv_s64 (53));

  /* Bounds x between zero_bound and inf_bound, where exp2(x) would return
     0 or inf. By clamping x to these bounds, the behaviour of large values
     is more predictable.  */
  x = svmin_x (pg, svmax_x (pg, x, d->zero_bound), d->inf_bound);

  svfloat64_t z = svadd_x (svptrue_b64 (), x, d->shift);
  svfloat64_t n = svsub_x (pg, z, d->shift);
  svfloat64_t r = svsub_x (pg, x, n);

  /* Computes scale with an offset, which returns scale = 2^(n - 53).  */
  z = svsub_x (pg, z, offset);
  svfloat64_t scale = svexpa (svreinterpret_u64 (z));

  svfloat64_t r2 = svmul_x (svptrue_b64 (), r, r);
  svfloat64_t c24 = svld1rq (svptrue_b64 (), &d->c2);

  /* Approximate exp2(r) using polynomial.  */
  /* y = exp2(r) - 1 ~= r * (C0 + C1 r + C2 r^2 + C3 r^3 + C4 r^4).  */
  svfloat64_t p12 = svmla_lane (sv_f64 (d->c1), r, c24, 0);
  svfloat64_t p34 = svmla_lane (sv_f64 (d->c3), r, c24, 1);
  svfloat64_t p = svmla_x (pg, p12, p34, r2);
  p = svmad_x (pg, p, r, d->c0);
  svfloat64_t poly = svmul_x (svptrue_b64 (), r, p);

  /* Reconstruct y as 2^(n - 53) * (1 + poly(r)).  */
  svfloat64_t y = svmla_x (pg, scale, scale, poly);

  /* Scale the result by 2^53:
    2^n * (1 + poly(r)) = 2^53 * 2^(n - 53) * (1 + poly(r)).  */
  return svscale_x (pg, y, scale_adjust);
}

/* Vector version of exp2
   The maximum observed error is 0.52 + 0.5 ULP.
   _ZGVsMxv_exp2 (0x1.3b72ad5b701bfp-1)
    got 0x1.8861641b49e08p+0
   want 0x1.8861641b49e07p+0.  */
svfloat64_t SV_NAME_D1 (exp2) (svfloat64_t x, svbool_t pg)
{
  const struct data *d = ptr_barrier (&data);
  svbool_t special = svacge (svptrue_b64 (), x, d->special_bound);
  if (unlikely (svptest_any (special, special)))
    return special_case (x, pg, d);
  return exp2_inline (x, pg, d);
}

TEST_SIG (SV, D, 1, exp2, -9.9, 9.9)
TEST_ULP (SV_NAME_D1 (exp2), 0.52)
TEST_SYM_INTERVAL (SV_NAME_D1 (exp2), 0, SpecialBound, 100000)
TEST_INTERVAL (SV_NAME_D1 (exp2), SpecialBound, InfBound, 10000)
TEST_INTERVAL (SV_NAME_D1 (exp2), -SpecialBound, ZeroBound, 10000)
TEST_INTERVAL (SV_NAME_D1 (exp2), InfBound, inf, 1000)
TEST_INTERVAL (SV_NAME_D1 (exp2), ZeroBound, -inf, 1000)
CLOSE_SVE_ATTR
