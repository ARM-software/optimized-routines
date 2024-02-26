/*
 * Double-precision Streaming Compatible SVE cos(x) function.
 *
 * Copyright (c) 2024, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "sv_math.h"
#include "pl_sig.h"
#include "pl_test.h"
#include "poly_sve_f64.h"

static const struct data
{
  double inv_pi, negpi1, negpi2, negpi3, half_pi, shift, range_val;
  double poly[7];
} data = {
  /* Coefficients for sin(x) approximation.  */
  .poly = { -0x1.555555555547bp-3, 0x1.1111111108a4dp-7, -0x1.a01a019936f27p-13,
            0x1.71de37a97d93ep-19, -0x1.ae633919987c6p-26,
            0x1.60e277ae07cecp-33, -0x1.9e9540300a1p-41, },

  .inv_pi = 0x1.45f306dc9c883p-2,
  .negpi1 = -0x1.921fb54442d18p+1,
  .negpi2 = -0x1.1a62633145c06p-53,
  .negpi3 = -0x1.c1cd129024e09p-106,
  .half_pi = 0x1.921fb54442d18p+0,
  .shift = 0x1.8p52,
  .range_val = 0x1p46,
};

static svfloat64_t NOINLINE
special_case (svbool_t pg, svfloat64_t x, svfloat64_t y, svuint64_t odd,
	      svbool_t cmp) SC_ATTR
{
  return sv_call_f64 (
      cos, x, svreinterpret_f64 (sveor_z (pg, svreinterpret_u64 (y), odd)),
      cmp);
}

/* A Streaming Compatible implementation of SVE cos.
   Maximum error 3.19 ULP:
   _ZGVsMxv_sc_cos(-0x1.9b256fdccc64p+32) got 0x1.fc3d78598056fp-3
					 want 0x1.fc3d78598056cp-3.  */
svfloat64_t SC_NAME_D1 (cos) (svfloat64_t x, const svbool_t pg) SC_ATTR
{
  const struct data *d = ptr_barrier (&data);
  svbool_t cmp = svacgt (pg, x, d->range_val);

  /* Load some values in quad-word chunks to minimise memory access.  */
  const svbool_t ptrue = svptrue_b64 ();
  svfloat64_t inv_pi_and_negpi1 = svld1rq (ptrue, &d->inv_pi);
  svfloat64_t negpi2_and_3 = svld1rq (ptrue, &d->negpi2);
  svfloat64_t shift = sv_f64 (d->shift);

  /* n = rint((|x|+pi/2)/pi) - 0.5.  */
  svfloat64_t r = x;
  svfloat64_t n = svadd_x (pg, x, sv_f64 (d->half_pi));
  n = svmla_lane (shift, n, inv_pi_and_negpi1, 0);
  svuint64_t odd = svlsl_x (pg, svreinterpret_u64 (n), 63);
  n = svsub_x (pg, n, shift);
  n = svsub_x (pg, n, sv_f64 (0.5));

  /* r = |x| - n*(pi/2)  (range reduction into -pi/2 .. pi/2).  */
  r = svmla_lane (r, n, inv_pi_and_negpi1, 1);
  r = svmla_lane (r, n, negpi2_and_3, 0);
  r = svmla_lane (r, n, negpi2_and_3, 1);

  /* sin(r) poly approx.  */
  svfloat64_t r2 = svmul_x (pg, r, r);
  svfloat64_t r3 = svmul_x (pg, r2, r);
  svfloat64_t r4 = svmul_x (pg, r2, r2);
  svfloat64_t y = sv_pw_horner_6_f64_x (pg, r2, r4, d->poly);
  y = svmla_x (pg, r, y, r3);

  if (unlikely (svptest_any (pg, cmp)))
    return special_case (pg, x, y, odd, cmp);

  /* Copy sign.  */
  return svreinterpret_f64 (sveor_z (pg, svreinterpret_u64 (y), odd));
}

PL_SIG (SC, D, 1, cos, -3.1, 3.1)
PL_TEST_ULP (SC_NAME_D1 (cos), 2.69)
PL_TEST_SYM_INTERVAL (SC_NAME_D1 (cos), 0, 0xffff0000, 10000)
PL_TEST_SYM_INTERVAL (SC_NAME_D1 (cos), 0x1p-4, 0x1p4, 500000)
PL_TEST_SYM_INTERVAL (SC_NAME_D1 (cos), 0x1p4, inf, 1000)
