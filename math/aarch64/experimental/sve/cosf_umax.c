/*
 * Low-accuracy single-precision SVE cos(x) function.
 *
 * Copyright (c) 2025, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "sv_math.h"
#include "test_defs.h"

static const struct data
{
  float range_val, c0;
  float inv_pi, negpi1, negpi2, c1;
} data = {
  .inv_pi = 0x1.45f306p-2f,
  .negpi1 = -0x1.921fb6p+1f,
  .negpi2 = 0x1.777a5cp-24f,
  /* Coefficients for sin(x) approximation.  */
  .c0 = -0x1.545304p-3f,
  .c1 = 0x1.f73186p-8f,
  /* The scalar fallback is still required for large values.
     The threshold is adapted to maintain accuracy to 4096 ULP.  */
  .range_val = 0x1p15f,
};

static svfloat32_t NOINLINE
special_case (svfloat32_t x, svfloat32_t y, svuint32_t odd, svbool_t pg,
	      svbool_t cmp)
{
  y = svreinterpret_f32 (sveor_x (pg, svreinterpret_u32 (y), odd));
  return sv_call_f32 (cosf, x, y, cmp);
}

/* A fast inaccurate SVE implementation of cosf.
   It is as fast but more accurate to use RINTA than a shift
   technique in this context.
   Maximum error: 2857.36 +0.5 ULP.
   arm_math_sve_fast_cosf(0x1.96586ap+14) got 0x1.000b28p+0
					 want 0x1.fffffcp-1.  */
svfloat32_t
arm_math_sve_fast_cosf (svfloat32_t x, const svbool_t pg)
{
  const struct data *d = ptr_barrier (&data);

  /* Branch is necessary to maintain accuracy on large values of |x|.  */
  svbool_t cmp = svacge (pg, x, d->range_val);

  /* Load some values in quad-word chunks to minimise memory access.  */
  svfloat32_t cts = svld1rq (svptrue_b32 (), &d->inv_pi);

  /* n = rint((|x|+pi/2)/pi) - 0.5.  */
  svfloat32_t n = svmla_lane (sv_f32 (0.5f), x, cts, 0);
  n = svrinta_x (pg, n);
  svuint32_t odd = svlsl_x (pg, svreinterpret_u32 (svcvt_s32_x (pg, n)), 31);
  n = svsub_x (pg, n, 0.5f);

  /* r = |x| - n*pi  (range reduction into -pi/2 .. pi/2).  */
  svfloat32_t r = x;
  r = svmla_lane (r, n, cts, 1);
  r = svmla_lane (r, n, cts, 2);

  /* y = sin(r).  */
  svfloat32_t r2 = svmul_x (svptrue_b32 (), r, r);
  svfloat32_t r3 = svmul_x (svptrue_b32 (), r, r2);
  svfloat32_t y = svmla_lane (sv_f32 (d->c0), r2, cts, 3);
  y = svmla_x (pg, r, y, r3);

  if (unlikely (svptest_any (pg, cmp)))
    return special_case (x, y, odd, pg, cmp);
  return svreinterpret_f32 (sveor_x (pg, svreinterpret_u32 (y), odd));
}

TEST_ULP (arm_math_sve_fast_cosf, 4096)
TEST_SYM_INTERVAL (arm_math_sve_fast_cosf, 0, 0x1p15f, 500000)
TEST_SYM_INTERVAL (arm_math_sve_fast_cosf, 0x1p15f, inf, 10000)
CLOSE_SVE_ATTR
