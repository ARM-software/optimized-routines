/*
 * Low-accuracy single-precision SVE sin(x) function.
 *
 * Copyright (c) 2025, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "sv_math.h"
#include "test_defs.h"

static const struct data
{
  float invpi, negpi1, negpi2, c1;
  float range_val, shift, c0;
} data = {
  .negpi1 = -0x1.921fb6p+1f,
  .negpi2 = 0x1.777a5cp-24f,
  .invpi = 0x1.45f306p-2f,
  .shift = 0x1.8p+23f,
  /* Coefficients for sin(x) approximation.  */
  .c0 = -0x1.545304p-3f,
  .c1 = 0x1.f73186p-8f,
  /* The scalar fallback is still required for large values.
     The threshold is adapted to maintain accuracy to 4096 ULP.  */
  .range_val = 0x1p16f,
};

static svfloat32_t NOINLINE
special_case (svfloat32_t x, svfloat32_t y, svuint32_t sign, svbool_t pg,
	      svbool_t cmp)
{
  y = svreinterpret_f32 (
      sveor_x (svnot_z (pg, cmp), svreinterpret_u32 (y), sign));
  return sv_call_f32 (sinf, x, y, cmp);
}

/* A fast inaccurate SVE implementation of sinf.
   It is slightly faster (less accurate) to perform RINTA using
   a shift-technique in this context.
   Maximum error: 2991.22 +0.5 ULPs.
   arm_math_sve_fast_sinf(0x1.ffacb6p+15) got -0x1.000b76p+0
					 want -0x1.ffff8cp-1.  */
svfloat32_t
arm_math_sve_fast_sinf (svfloat32_t x, const svbool_t pg)
{
  const struct data *d = ptr_barrier (&data);

  svfloat32_t ax = svabs_x (pg, x);
  svuint32_t sign
      = sveor_x (pg, svreinterpret_u32 (x), svreinterpret_u32 (ax));

  /* Branch is necessary to maintain accuracy on large values of |x|.  */
  svbool_t cmp = svacgt (pg, x, d->range_val);

  svfloat32_t cts = svld1rq (svptrue_b32 (), &d->invpi);

  /* n = rint(|x|/pi).  */
  svfloat32_t n = svmla_lane (sv_f32 (d->shift), ax, cts, 0);
  svuint32_t odd = svlsl_x (pg, svreinterpret_u32 (n), 31);
  n = svsub_x (pg, n, d->shift);

  /* r = |x| - n*pi  (range reduction into -pi/2 .. pi/2).  */
  svfloat32_t r = ax;
  r = svmla_lane (r, n, cts, 1);
  r = svmla_lane (r, n, cts, 2);

  /* sin(r) approx using a degree-1 polynomial in r^2.  */
  svfloat32_t r2 = svmul_x (svptrue_b32 (), r, r);
  svfloat32_t r3 = svmul_x (svptrue_b32 (), r, r2);
  svfloat32_t y = svmla_lane (sv_f32 (d->c0), r2, cts, 3);
  y = svmla_x (pg, r, r3, y);

  /* sign = y^sign^odd.  */
  sign = sveor_x (pg, sign, odd);

  if (unlikely (svptest_any (pg, cmp)))
    return special_case (x, y, sign, pg, cmp);
  return svreinterpret_f32 (sveor_x (pg, svreinterpret_u32 (y), sign));
}

TEST_ULP (arm_math_sve_fast_sinf, 4096)
TEST_DISABLE_FENV (arm_math_sve_fast_sinf)
TEST_SYM_INTERVAL (arm_math_sve_fast_sinf, 0, 0x1p16f, 500000)
TEST_SYM_INTERVAL (arm_math_sve_fast_sinf, 0x1p16f, inf, 10000)
CLOSE_SVE_ATTR
