/*
 * Double-precision SVE 2^x function.
 *
 * Copyright (c) 2023, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "sv_math.h"
#include "sv_estrin.h"
#include "pl_sig.h"
#include "pl_test.h"

#if SV_SUPPORTED

#define N (1 << V_EXP2_TABLE_BITS)

#define UFlowBound 0x1p-512
#define OFlowBound 0x1p9

static struct __sv_exp2_data
{
  double poly[V_EXP2_POLY_ORDER];
  double uflow_bound, oflow_bound, shift;
} data = {
  /* Lower bound, below which algorithm spuriously underflows.  */
  .uflow_bound = UFlowBound,
  /* Upper bound, above which algorithm spuriously overflows.  */
  .oflow_bound = OFlowBound,
  .shift = 0x1.8p52 / N,
  .poly = {0x1.62e42fefa39efp-1, 0x1.ebfbdff82c424p-3, 0x1.c6b08d70cf4b5p-5,
	   0x1.3b2abd24650ccp-7, 0x1.5d7e09b4e3a84p-10},
};

#define C(i) sv_f64 (data.poly[i])

static svfloat64_t NOINLINE
special_case (svfloat64_t x, svfloat64_t y, svbool_t pg)
{
  return sv_call_f64 (exp2, x, y, pg);
}

/* Vector implementation of exp2. Maximum observed error is 1.00 ULP:
   _ZGVsMxv_exp2(-0x1.8c68c2231567ap-2) got 0x1.8780e9885f8e3p-1
				       want 0x1.8780e9885f8e2p-1.  */
svfloat64_t SV_NAME_D1 (exp2) (svfloat64_t x, svbool_t pg)
{
  svbool_t no_uflow = svacge_n_f64 (pg, x, data.uflow_bound);
  svbool_t no_oflow_or_infnan = svacle_n_f64 (pg, x, data.oflow_bound);
  svbool_t special = svnand_b_z (pg, no_uflow, no_oflow_or_infnan);

  /* Reduce x to k/N + r, where k is integer and r in [-1/2N, 1/2N].  */
  svfloat64_t shift = sv_f64 (data.shift);
  svfloat64_t kd = svadd_f64_x (pg, x, shift);
  svuint64_t ki = svreinterpret_u64_f64 (kd);
  /* kd = k/N.  */
  kd = svsub_f64_x (pg, kd, shift);
  svfloat64_t r = svsub_f64_x (pg, x, kd);

  /* scale ~= 2^(k/N). Get scale bottom from table, with index n % N. Since N
     is a power of 2, n % N = n & (N - 1).  */
  svuint64_t idx = svand_n_u64_x (pg, ki, N - 1);
  svuint64_t tab_sbits = svld1_gather_u64index_u64 (pg, __v_exp2_data, idx);
  svuint64_t top = svlsl_n_u64_x (pg, ki, 52 - V_EXP2_TABLE_BITS);
  svuint64_t sbits = svadd_u64_x (pg, tab_sbits, top);
  /* This is only a valid scale when -1023*N < k < 1024*N.  */
  svfloat64_t scale = svreinterpret_f64_u64 (sbits);

  /* Approximate exp2(r) using polynomial. Use offset version of Estrin wrapper
     to evaluate from C1 onwards, and avoid forming r2*r2 to avoid overflow.  */
  svfloat64_t r2 = svmul_f64_x (pg, r, r);
  svfloat64_t p1234 = ESTRIN_3_ (pg, r, r2, C, 1);
  svfloat64_t p0 = svmul_f64_x (pg, r, C (0));
  svfloat64_t exp2_r = svmla_f64_x (pg, p0, r2, p1234);

  /* Assemble exp2(x) = exp2(r) * scale.  */
  svfloat64_t y = svmla_f64_x (pg, scale, scale, exp2_r);

  if (unlikely (svptest_any (pg, special)))
    return special_case (x, y, pg);
  return y;
}

PL_SIG (SV, D, 1, exp2, -9.9, 9.9)
PL_TEST_ULP (SV_NAME_D1 (exp2), 0.51)
PL_TEST_INTERVAL (SV_NAME_D1 (exp2), 0, UFlowBound, 1000)
PL_TEST_INTERVAL (SV_NAME_D1 (exp2), UFlowBound, OFlowBound, 100000)
PL_TEST_INTERVAL (SV_NAME_D1 (exp2), OFlowBound, inf, 1000)
PL_TEST_INTERVAL (SV_NAME_D1 (exp2), -0, -UFlowBound, 1000)
PL_TEST_INTERVAL (SV_NAME_D1 (exp2), -UFlowBound, -OFlowBound, 100000)
PL_TEST_INTERVAL (SV_NAME_D1 (exp2), -OFlowBound, -inf, 1000)
#endif
