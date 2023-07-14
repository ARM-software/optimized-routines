/*
 * Double-precision vector exp(x) - 1 function.
 *
 * Copyright (c) 2023, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "sv_math.h"
#include "poly_sve_f64.h"
#include "pl_sig.h"
#include "pl_test.h"

#define SV_EXPM1_POLY_ORDER 10
#define SpecialBound 0x1.62b7d369a5aa9p+9
#define ExponentBias 0x3ff0000000000000

static const struct data
{
  double poly[SV_EXPM1_POLY_ORDER + 1];
  double shift, inv_ln2, special_bound;
  /* To be loaded in one quad-word.  */
  double ln2_hi, ln2_lo;
} data = {.special_bound = SpecialBound,
	  .inv_ln2 = 0x1.71547652b82fep0,
	  .ln2_hi = 0x1.62e42fefa39efp-1,
	  .ln2_lo = 0x1.abc9e3b39803fp-56,
	  .shift = 0x1.8p52,
	  /* Generated using fpminimax, see tools/expm1.sollya for details.  */
	  .poly = {0x1p-1, 0x1.5555555555559p-3, 0x1.555555555554bp-5,
		   0x1.111111110f663p-7, 0x1.6c16c16c1b5f3p-10,
		   0x1.a01a01affa35dp-13, 0x1.a01a018b4ecbbp-16,
		   0x1.71ddf82db5bb4p-19, 0x1.27e517fc0d54bp-22,
		   0x1.af5eedae67435p-26, 0x1.1f143d060a28ap-29}};

static svfloat64_t NOINLINE
special_case (svfloat64_t x, svfloat64_t y, svbool_t pg)
{
  return sv_call_f64 (expm1, x, y, pg);
}

/* Double-precision vector exp(x) - 1 function.
   The maximum error observed error is 2.18 ULP:
   _ZGVsMxv_expm1(0x1.634ba0c237d7bp-2) got 0x1.a8b9ea8d66e22p-2
				       want 0x1.a8b9ea8d66e2p-2.  */
svfloat64_t SV_NAME_D1 (expm1) (svfloat64_t x, svbool_t pg)
{
  const struct data *d = ptr_barrier (&data);
  /* Large, Nan/Inf.  */
  svbool_t special = svnot_b_z (pg, svaclt_n_f64 (pg, x, d->special_bound));
  /* Argument reduction discards sign of zero, but expm1(-0) is expected to be
     -0. Use merging predication on zero lanes to propagate -0 correctly.  */
  svbool_t pnz = svcmpne_n_f64 (pg, x, 0);

  /* Reduce argument to smaller range:
     Let i = round(x / ln2)
     and f = x - i * ln2, then f is in [-ln2/2, ln2/2].
     exp(x) - 1 = 2^i * (expm1(f) + 1) - 1
     where 2^i is exact because i is an integer.  */
  svfloat64_t shift = sv_f64 (d->shift);
  svfloat64_t n
      = svsub_f64_x (pg, svmla_n_f64_x (pg, shift, x, d->inv_ln2), shift);
  svint64_t i = svcvt_s64_f64_m (svreinterpret_s64_f64 (x), pnz, n);
  svfloat64_t ln2 = svld1rq_f64 (svptrue_b64 (), &d->ln2_hi);
  svfloat64_t f = svmls_lane_f64 (x, n, ln2, 0);
  f = svmls_lane_f64 (f, n, ln2, 1);

  /* Approximate expm1(f) using polynomial.
     Taylor expansion for expm1(x) has the form:
	 x + ax^2 + bx^3 + cx^4 ....
     So we calculate the polynomial P(f) = a + bf + cf^2 + ...
     and assemble the approximation expm1(f) ~= f + f^2 * P(f).  */
  svfloat64_t f2 = svmul_f64_x (pg, f, f), f4 = svmul_f64_x (pg, f2, f2),
	      f8 = svmul_f64_x (pg, f4, f4);
  svfloat64_t p = svmla_f64_x (
      pg, f, f2, sv_estrin_10_f64_x (pg, f, f2, f4, f8, d->poly));

  /* Assemble the result.
   expm1(x) ~= 2^i * (p + 1) - 1
   Let t = 2^i.  */
  svint64_t u = svadd_n_s64_m (pnz, svlsl_n_s64_m (pnz, i, 52), ExponentBias);
  svfloat64_t t = svreinterpret_f64_s64 (u);

  /* expm1(x) ~= p * t + (t - 1).  */
  svfloat64_t y = svmla_f64_m (pnz, svsub_n_f64_m (pnz, t, 1), p, t);

  if (unlikely (svptest_any (pg, special)))
    return special_case (x, y, special);

  return y;
}

PL_SIG (SV, D, 1, expm1, -9.9, 9.9)
PL_TEST_ULP (SV_NAME_D1 (expm1), 1.68)
PL_TEST_INTERVAL (SV_NAME_D1 (expm1), 0, 0x1p-23, 1000)
PL_TEST_INTERVAL (SV_NAME_D1 (expm1), -0, -0x1p-23, 1000)
PL_TEST_INTERVAL (SV_NAME_D1 (expm1), 0x1p-23, SpecialBound, 200000)
PL_TEST_INTERVAL (SV_NAME_D1 (expm1), -0x1p-23, -SpecialBound, 200000)
PL_TEST_INTERVAL (SV_NAME_D1 (expm1), SpecialBound, inf, 1000)
PL_TEST_INTERVAL (SV_NAME_D1 (expm1), -SpecialBound, -inf, 1000)
