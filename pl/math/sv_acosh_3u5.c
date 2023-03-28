/*
 * Double-precision SVE acosh(x) function.
 * Copyright (c) 2023, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "sv_math.h"
#include "pl_sig.h"
#include "pl_test.h"

#if SV_SUPPORTED

#define WANT_SV_LOG1P_K0_SHORTCUT 1
#include "sv_log1p_inline.h"

#define BigBoundTop 0x5fe /* top12 (asuint64 (0x1p511)).  */
#define OneTop 0x3ff

static NOINLINE svfloat64_t
special_case (svfloat64_t x, svfloat64_t y, svbool_t special)
{
  return sv_call_f64 (acosh, x, y, special);
}

/* SVE approximation for double-precision acosh, based on log1p.
   The largest observed error is 3.01 ULP in the region where the
   argument to log1p falls in the k=0 interval, i.e. x close to 1:
   __sv_acosh(0x1.007ba383a7b15p+0) got 0x1.f71f6ed80b9bep-5
				   want 0x1.f71f6ed80b9c1p-5.  */
svfloat64_t SV_NAME_D1 (acosh) (svfloat64_t x, const svbool_t pg)
{
  svuint64_t itop = svlsr_n_u64_x (pg, svreinterpret_u64_f64 (x), 52);
  /* (itop - OneTop) >= (BigBoundTop - OneTop).  */
  svbool_t special
    = svcmpge (pg, svsub_n_u64_x (pg, itop, OneTop), sv_u64 (0x1ff));

  svfloat64_t xm1 = svsub_n_f64_x (pg, x, 1);
  svfloat64_t u = svmul_f64_x (pg, xm1, svadd_n_f64_x (pg, x, 1));
  svfloat64_t y
    = sv_log1p_inline (svadd_f64_x (pg, xm1, svsqrt_f64_x (pg, u)), pg);

  /* Fall back to scalar routine for special lanes.  */
  if (unlikely (svptest_any (pg, special)))
    return special_case (x, y, special);

  return y;
}

PL_SIG (SV, D, 1, acosh, 1.0, 10.0)
PL_TEST_ULP (SV_NAME_D1 (acosh), 2.51)
PL_TEST_INTERVAL (SV_NAME_D1 (acosh), 1, 0x1p511, 90000)
PL_TEST_INTERVAL (SV_NAME_D1 (acosh), 0x1p511, inf, 10000)
PL_TEST_INTERVAL (SV_NAME_D1 (acosh), 0, 1, 1000)
PL_TEST_INTERVAL (SV_NAME_D1 (acosh), -0, -inf, 10000)
#endif
