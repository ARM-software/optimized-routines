/*
 * Single-precision vector exp(x) - 1 function.
 *
 * Copyright (c) 2023-2025, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "sv_math.h"
#include "test_sig.h"
#include "test_defs.h"

/* Largest value of x for which expm1(x) should round to -1.  */
#define SpecialBound 0x1.5ebc4p+6f

/* Value of n above which scale overflows even with special treatment.  */
#define ScaleBound 192.0f

static const struct data
{
  /* These 4 are grouped together so they can be loaded as one quadword, then
     used with _lane forms of svmla/svmls.  */
  float c2, c4, ln2_hi, ln2_lo;
  float c0, inv_ln2, c1, c3;
  float scale_thresh, special_bound;
  uint32_t special_offset, special_bias;
} data = {
  /* Generated using fpminimax.  */
  .ln2_hi = 0x1.62e4p-1f,	.ln2_lo = 0x1.7f7d1cp-20f,
  .c0 = 0x1.fffffep-2,		.c1 = 0x1.5554aep-3,
  .c2 = 0x1.555736p-5,		.c3 = 0x1.12287cp-7,
  .c4 = 0x1.6b55a2p-10,		.inv_ln2 = 0x1.715476p+0f,
  .special_bias = 0x7f000000,	.scale_thresh = ScaleBound,
  .special_offset = 0x82000000, .special_bound = SpecialBound,

};

/* Special case routine shared with other exponentials.  */
static inline svfloat32_t
special_exp (svfloat32_t poly, svfloat32_t n, svuint32_t e, svbool_t cmp1,
	     svfloat32_t scale, const struct data *d)
{
  svbool_t b = svcmple (svptrue_b32 (), n, 0.0f);
  svfloat32_t s1 = svreinterpret_f32 (
      svsel (b, sv_u32 (d->special_offset + d->special_bias),
	     sv_u32 (d->special_bias)));
  svfloat32_t s2
      = svreinterpret_f32 (svsub_m (b, e, sv_u32 (d->special_offset)));
  svbool_t cmp2 = svacgt (svptrue_b32 (), n, d->scale_thresh);
  svfloat32_t r2 = svmul_x (svptrue_b32 (), s1, s1);
  svfloat32_t r1
      = svmul_x (svptrue_b32 (), svmla_x (svptrue_b32 (), s2, poly, s2), s1);
  svfloat32_t r0 = svmla_x (svptrue_b32 (), scale, poly, scale);
  svfloat32_t r = svsel (cmp1, r1, r0);
  return svsel (cmp2, r2, r);
}

/* Special case routine for expm1.  */
static svfloat32_t NOINLINE
special_case (svfloat32_t poly, svfloat32_t n, svbool_t cmp1,
	      svfloat32_t scale, const struct data *d)
{
  /* Compute unbiased exponent of scale.  */
  svuint32_t e = svlsl_x (
      svptrue_b32 (), svreinterpret_u32 (svcvt_s32_x (svptrue_b32 (), n)), 23);
  /* compute special exp and subtract 1.  */
  svfloat32_t special = svsub_x (
      svptrue_b32 (), special_exp (poly, n, e, cmp1, scale, d), 1.0f);
  /* compute non-special output.  */
  svfloat32_t y = svmla_x (svptrue_b32 (),
			   svsub_x (svptrue_b32 (), scale, 1.0f), poly, scale);
  return svsel_f32 (cmp1, special, y);
}

/* Single-precision SVE exp(x) - 1.
   Maximum error is 1.02 +0.5ULP:
   _ZGVsMxv_expm1f(0x1.8f4ebcp-2) got 0x1.e859dp-2
				 want 0x1.e859d4p-2.  */
svfloat32_t SV_NAME_F1 (expm1) (svfloat32_t x, const svbool_t pg)
{
  const struct data *d = ptr_barrier (&data);

  /* This vector is reliant on layout of data - it contains constants
     that can be used with _lane forms of svmla/svmls. Values are:
     [ coeff_2, coeff_4, ln2_hi, ln2_lo ].  */
  svfloat32_t lane_constants = svld1rq (svptrue_b32 (), &d->c2);

  /* Reduce argument to smaller range:
     Let i = round(x / ln2)
     and f = x - i * ln2, then f is in [-ln2/2, ln2/2].
     exp(x) - 1 = 2^i * (expm1(f) + 1) - 1
     where 2^i is exact because i is an integer.  */
  svfloat32_t n = svrinta_x (pg, svmul_x (svptrue_b32 (), x, d->inv_ln2));

  svfloat32_t f = svmls_lane_f32 (x, n, lane_constants, 2);
  f = svmls_lane_f32 (f, n, lane_constants, 3);

  /* Approximate expm1(f) using polynomial.
     Taylor expansion for expm1(x) has the form:
     x + ax^2 + bx^3 + cx^4 ....
     So we calculate the polynomial P(f) = a + bf + cf^2 + ...
     and assemble the approximation expm1(f) ~= f + f^2 * P(f).  */
  svfloat32_t p12 = svmla_lane (sv_f32 (d->c1), f, lane_constants, 0);
  svfloat32_t p34 = svmla_lane (sv_f32 (d->c3), f, lane_constants, 1);
  svfloat32_t f2 = svmul_x (svptrue_b32 (), f, f);

  svfloat32_t poly = svmla_x (pg, p12, f2, p34);
  poly = svmla_x (pg, sv_f32 (d->c0), f, poly);
  poly = svmla_x (pg, f, f2, poly);

  /* Assemble the result.
     expm1(x) ~= scale * (poly + 1) - 1
     with scale = 2^i.  */
  svfloat32_t scale = svscale_x (pg, sv_f32 (1.0f), svcvt_s32_x (pg, n));

  /* Large, NaN/Inf.  */
  svbool_t cmp = svacgt_n_f32 (pg, x, d->special_bound);

  if (unlikely (svptest_any (pg, cmp)))
    return special_case (poly, n, cmp, scale, d);

  return svmla_x (pg, svsub_x (pg, scale, 1.0f), poly, scale);
}

TEST_SIG (SV, F, 1, expm1, -9.9, 9.9)
TEST_ULP (SV_NAME_F1 (expm1), 1.02)
TEST_SYM_INTERVAL (SV_NAME_F1 (expm1), 0, SpecialBound, 100000)
TEST_SYM_INTERVAL (SV_NAME_F1 (expm1), SpecialBound, inf, 1000)
CLOSE_SVE_ATTR
