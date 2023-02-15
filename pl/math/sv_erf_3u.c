/*
 * Double-precision SVE erf(x) function.
 *
 * Copyright (c) 2020-2023, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "sv_math.h"
#include "pl_sig.h"
#include "pl_test.h"

#if SV_SUPPORTED

#define Scale (8.0)
#define AbsMask (0x7fffffffffffffff)

static NOINLINE svfloat64_t
__sv_erf_specialcase (svfloat64_t x, svfloat64_t y, svbool_t cmp)
{
  return sv_call_f64 (erf, x, y, cmp);
}

/* Optimized double precision SVE error function erf.
   Maximum observed error is 2.62 ULP:
   SV_NAME_D1 (erf)(0x1.79cab7e3078fap+2) got 0x1.0000000000001p+0
				 want 0x1.fffffffffffffp-1.  */
svfloat64_t SV_NAME_D1 (erf) (svfloat64_t x, const svbool_t pg)
{
  /* Use top 16 bits to test for special cases and small values.  */
  svuint64_t ix = sv_as_u64_f64 (x);
  svuint64_t atop = svand_n_u64_x (pg, svlsr_n_u64_x (pg, ix, 48), 0x7fff);

  /* Handle both inf/nan as well as small values (|x|<2^-28).  */
  svbool_t cmp
    = svcmpge_n_u64 (pg, svsub_n_u64_x (pg, atop, 0x3e30), 0x7ff0 - 0x3e30);

  /* Get sign and absolute value.  */
  svfloat64_t a = sv_as_f64_u64 (svand_n_u64_x (pg, ix, AbsMask));
  svuint64_t sign = svand_n_u64_x (pg, ix, ~AbsMask);

  /* i = trunc(Scale*x).  */
  svfloat64_t a_scale = svmul_n_f64_x (pg, a, Scale);
  /* Saturate index of intervals.  */
  svbool_t a_lt_6 = svcmplt_n_u64 (pg, atop, 0x4018);
  svuint64_t i = svcvt_u64_f64_m (sv_u64 (V_ERF_NINTS - 1), a_lt_6, a_scale);

  /* Load polynomial coefficients.  */
  svfloat64_t P_0 = svld1_gather_u64index_f64 (pg, __v_erf_data.coeffs[0], i);
  svfloat64_t P_1 = svld1_gather_u64index_f64 (pg, __v_erf_data.coeffs[1], i);
  svfloat64_t P_2 = svld1_gather_u64index_f64 (pg, __v_erf_data.coeffs[2], i);
  svfloat64_t P_3 = svld1_gather_u64index_f64 (pg, __v_erf_data.coeffs[3], i);
  svfloat64_t P_4 = svld1_gather_u64index_f64 (pg, __v_erf_data.coeffs[4], i);
  svfloat64_t P_5 = svld1_gather_u64index_f64 (pg, __v_erf_data.coeffs[5], i);
  svfloat64_t P_6 = svld1_gather_u64index_f64 (pg, __v_erf_data.coeffs[6], i);
  svfloat64_t P_7 = svld1_gather_u64index_f64 (pg, __v_erf_data.coeffs[7], i);
  svfloat64_t P_8 = svld1_gather_u64index_f64 (pg, __v_erf_data.coeffs[8], i);
  svfloat64_t P_9 = svld1_gather_u64index_f64 (pg, __v_erf_data.coeffs[9], i);

  /* Get shift and scale.  */
  svfloat64_t shift = svld1_gather_u64index_f64 (pg, __v_erf_data.shifts, i);

  /* Transform polynomial variable.
     Set z = 0 in the boring domain to avoid overflow.  */
  svfloat64_t z = svmla_f64_m (a_lt_6, shift, sv_f64 (Scale), a);

  /* Evaluate polynomial P(z) using level-2 Estrin.  */
  svfloat64_t r1 = svmla_f64_x (pg, P_0, P_1, z);
  svfloat64_t r2 = svmla_f64_x (pg, P_2, P_3, z);
  svfloat64_t r3 = svmla_f64_x (pg, P_4, P_5, z);
  svfloat64_t r4 = svmla_f64_x (pg, P_6, P_7, z);
  svfloat64_t r5 = svmla_f64_x (pg, P_8, P_9, z);

  svfloat64_t z2 = svmul_f64_x (pg, z, z);
  svfloat64_t z4 = svmul_f64_x (pg, z2, z2);

  svfloat64_t q2 = svmla_f64_x (pg, r3, z2, r4);
  svfloat64_t q1 = svmla_f64_x (pg, r1, z2, r2);

  svfloat64_t y = svmla_f64_x (pg, q2, r5, z4);
  y = svmla_f64_x (pg, q1, y, z4);

  /* y = erf(x) if x > 0, -erf(-x) otherwise.  */
  y = sv_as_f64_u64 (sveor_u64_x (pg, sv_as_u64_f64 (y), sign));

  if (unlikely (svptest_any (pg, cmp)))
    return __sv_erf_specialcase (x, y, cmp);
  return y;
}

PL_SIG (SV, D, 1, erf, -4.0, 4.0)
PL_TEST_ULP (SV_NAME_D1 (erf), 2.13)
PL_TEST_INTERVAL (SV_NAME_D1 (erf), 0, 0x1p-28, 20000)
PL_TEST_INTERVAL (SV_NAME_D1 (erf), 0x1p-28, 1, 60000)
PL_TEST_INTERVAL (SV_NAME_D1 (erf), 1, 0x1p28, 60000)
PL_TEST_INTERVAL (SV_NAME_D1 (erf), 0x1p28, inf, 20000)
PL_TEST_INTERVAL (SV_NAME_D1 (erf), -0, -0x1p-28, 20000)
PL_TEST_INTERVAL (SV_NAME_D1 (erf), -0x1p-28, -1, 60000)
PL_TEST_INTERVAL (SV_NAME_D1 (erf), -1, -0x1p28, 60000)
PL_TEST_INTERVAL (SV_NAME_D1 (erf), -0x1p28, -inf, 20000)
#endif
