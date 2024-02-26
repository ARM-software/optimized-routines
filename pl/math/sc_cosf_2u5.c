/*
 * Single-precision Streaming Compatible SVE cos(x) function.
 *
 * Copyright (c) 2024, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "sv_math.h"
#include "pl_sig.h"
#include "pl_test.h"
#include "poly_sve_f32.h"

static const struct data
{
  float poly[4];
  float range_val, half_pi, shift, inv_pi, negpi1, negpi2, negpi3;
} data = {
  /* Coefficients for sin(x) approximation.  */
  .poly = { -0x1.555548p-3f, 0x1.110df4p-7f, -0x1.9f42eap-13f,
             0x1.5b2e76p-19f, },

  .inv_pi = 0x1.45f306p-2f,
  .negpi1 = -0x1.921fb6p+1f,
  .negpi2 = 0x1.777a5cp-24f,
  .negpi3 = 0x1.ee59dap-49f,

  .shift = 0x1.8p+23f,
  .half_pi = 0x1.921fb6p0f,
  .range_val = 0x1p21f,
};

static svfloat32_t NOINLINE
special_case (svbool_t pg, svfloat32_t x, svfloat32_t y, svuint32_t odd,
	      svbool_t cmp) SC_ATTR
{
  return sv_call_f32 (
      cosf, x, svreinterpret_f32 (sveor_z (pg, svreinterpret_u32 (y), odd)),
      cmp);
}

/* A Streaming Compatible implementation of SVE cosf.
   Maximum error 2.48 ULP:
   _ZGVsMxv_sc_cosf (-0x1.ed31acp+20) got -0x1.fb6a52p-1
				     want -0x1.fb6a4ep-1.  */
svfloat32_t SC_NAME_F1 (cos) (svfloat32_t x, const svbool_t pg) SC_ATTR
{
  const struct data *d = ptr_barrier (&data);
  svbool_t cmp = svacgt (pg, x, d->range_val);

  /* Load some values in quad-word chunks to minimise memory access.  */
  svfloat32_t pi_vals = svld1rq (svptrue_b32 (), &d->inv_pi);
  svfloat32_t shift = sv_f32 (d->shift);

  /* n = rint((|x|+pi/2)/pi) - 0.5.  */
  svfloat32_t r = x;
  svfloat32_t xpi = svadd_x (pg, x, sv_f32 (d->half_pi));
  svfloat32_t n = svmla_lane (shift, xpi, pi_vals, 0);
  svuint32_t odd = svlsl_x (pg, svreinterpret_u32 (n), 31);
  n = svsub_x (pg, n, shift);
  n = svsub_x (pg, n, sv_f32 (0.5f));

  /* r = |x| - n*pi  (range reduction into -pi/2 .. pi/2).  */
  r = svmla_lane (r, n, pi_vals, 1);
  r = svmla_lane (r, n, pi_vals, 2);
  r = svmla_lane (r, n, pi_vals, 3);

  /* y = sin(r).  */
  svfloat32_t r2 = svmul_x (pg, r, r);
  svfloat32_t r3 = svmul_x (pg, r2, r);
  svfloat32_t y = sv_horner_3_f32_x (pg, r2, d->poly);
  y = svmla_x (pg, r, y, r3);

  if (unlikely (svptest_any (pg, cmp)))
    return special_case (pg, x, y, odd, cmp);

  return svreinterpret_f32 (sveor_x (pg, svreinterpret_u32 (y), odd));
}

PL_SIG (SC, F, 1, cos, -3.1, 3.1)
PL_TEST_ULP (SC_NAME_F1 (cos), 1.99)
PL_TEST_SYM_INTERVAL (SC_NAME_F1 (cos), 0, 0xffff0000, 10000)
PL_TEST_SYM_INTERVAL (SC_NAME_F1 (cos), 0x1p-4, 0x1p4, 500000)
PL_TEST_SYM_INTERVAL (SC_NAME_F1 (cos), 0x1p4, inf, 1000)
