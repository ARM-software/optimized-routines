/*
 * Single-precision vector 10^x - 1 function.
 *
 * Copyright (c) 2025, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "test_defs.h"
#include "sv_math.h"

#define ScaleBound 192.0f
#define SpecialBound 126.0f

static const struct data
{
  float log10_2_high, log10_2_low;
  float log10_lo, c2, c4, c6, c8;
  float32_t log10_hi, c1, c3, c5, c7;
  float32_t inv_log10_2, special_bound;
  uint32_t exponent_bias, special_offset, special_bias;
  float32_t scale_thresh;
} data = {
  /* Coefficients generated using Remez algorithm with minimisation of relative
     error.  */
  .log10_hi = 0x1.26bb1b8000000p+1,
  .log10_lo = 0x1.daaa8b0000000p-26,
  .c1 = 0x1.53524ep1,
  .c2 = 0x1.046fc8p1,
  .c3 = 0x1.2bd376p0,
  .c4 = 0x1.156f8p-1,
  .c5 = 0x1.b28c0ep-3,
  .c6 = -0x1.05e38ep-4,
  .c7 = -0x1.c79f4ap-4,
  .c8 = 0x1.2d6f34p1,
  .inv_log10_2 = 0x1.a934fp+1,
  .log10_2_high = 0x1.344136p-2,
  .log10_2_low = 0x1.ec10cp-27,
  /* rint (log2 (2^127 / (1 + sqrt (2)))).  */
  .special_bound = SpecialBound,
  .exponent_bias = 0x3f800000,
  .special_offset = 0x82000000,
  .special_bias = 0x7f000000,
  .scale_thresh = ScaleBound
};

static svfloat32_t NOINLINE
special_case (svfloat32_t poly, svfloat32_t n, svuint32_t e, svbool_t cmp1,
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
  return svsub_x (svptrue_b32 (), svsel (cmp2, r2, r), 1.0f);
}

/* Fast vector implementation of single-precision exp10.
   Algorithm is accurate to 1.68 + 0.5 ULP.
   _ZGVnN4v_exp10m1f(0x1.3aeffep-3) got 0x1.b3139p-2
				   want 0x1.b3138cp-2.  */
svfloat32_t SV_NAME_F1 (exp10m1) (svfloat32_t x, const svbool_t pg)
{
  const struct data *d = ptr_barrier (&data);

  /* exp10(x) = 2^n * 10^r = 2^n * (1 + poly (r)),
     with poly(r) in [1/sqrt(2), sqrt(2)] and
     x = r + n * log10 (2), with r in [-log10(2)/2, log10(2)/2].  */
  svfloat32_t log10_2 = svld1rq (svptrue_b32 (), &d->log10_2_high);
  svfloat32_t n = svrinta_x (pg, svmul_x (pg, x, d->inv_log10_2));
  svfloat32_t r = svmls_lane_f32 (x, n, log10_2, 0);
  r = svmla_lane_f32 (r, n, log10_2, 1);

  svuint32_t e = svlsl_x (pg, svreinterpret_u32 (svcvt_s32_x (pg, n)), 23);

  svfloat32_t scale
      = svreinterpret_f32 (svadd_n_u32_x (pg, e, d->exponent_bias));
  svbool_t cmp = svacgt_n_f32 (pg, n, d->special_bound);

  /* Pairwise Horner scheme.  */
  svfloat32_t r2 = svmul_x (pg, r, r);
  svfloat32_t c2468 = svld1rq (svptrue_b32 (), &d->c2);
  svfloat32_t p78 = svmla_lane (sv_f32 (d->c7), r, c2468, 3);
  svfloat32_t p56 = svmla_lane (sv_f32 (d->c5), r, c2468, 2);
  svfloat32_t p34 = svmla_lane (sv_f32 (d->c3), r, c2468, 1);
  svfloat32_t p12 = svmla_lane (sv_f32 (d->c1), r, c2468, 0);
  svfloat32_t p58 = svmla_x (pg, p56, r2, p78);
  svfloat32_t p36 = svmla_x (pg, p34, r2, p58);
  svfloat32_t p16 = svmla_x (pg, p12, r2, p36);

  svfloat32_t poly = svmla_n_f32_x (pg, svmul_x (pg, r, sv_f32 (d->log10_hi)),
				    r, d->log10_lo);
  poly = svmla_x (pg, poly, p16, r2);

  svfloat32_t ret = svmla_x (pg, svsub_x (pg, scale, 1.0f), poly, scale);
  if (unlikely (svptest_any (pg, cmp)))
    return svsel_f32 (cmp, special_case (poly, n, e, cmp, scale, d), ret);

  return ret;
}

#if WANT_C23_TESTS
TEST_ULP (SV_NAME_F1 (exp10m1), 1.68)
TEST_DISABLE_FENV (SV_NAME_F1 (exp10m1))
TEST_INTERVAL (SV_NAME_F1 (exp10m1), 0, 0xffff0000, 10000)
TEST_SYM_INTERVAL (SV_NAME_F1 (exp10m1), 0x1p-14, 0x1p-1, 50000)
TEST_SYM_INTERVAL (SV_NAME_F1 (exp10m1), 0x1p-1, 0x1p0, 5000)
TEST_SYM_INTERVAL (SV_NAME_F1 (exp10m1), 0x1p0, 0x1p1, 5000)
TEST_SYM_INTERVAL (SV_NAME_F1 (exp10m1), 0x1p1, 0x1p8, 5000)
#endif
CLOSE_SVE_ATTR
