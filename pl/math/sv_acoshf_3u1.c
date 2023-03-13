/*
 * Single-precision SVE acosh(x) function.
 * Copyright (c) 2023, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "sv_math.h"
#include "pl_sig.h"
#include "pl_test.h"

#if SV_SUPPORTED

#define SignMask 0x80000000
#define One 0x3f800000
#define SquareLim 0x5f800000 /* asuint(0x1p64).  */
#define Thres 0x20000000     /* SquareLim - One.  */

#include "sv_log1pf_inline.h"

static NOINLINE svfloat32_t
special_case (svfloat32_t x, svfloat32_t y, svbool_t special)
{
  return sv_call_f32 (acoshf, x, y, special);
}

/* Single-precision SVE acosh(x) routine. Implements the same algorithm as
   vector acoshf and log1p.

   Maximum error is 3.07 ULPs:
   SV_NAME_F1 (acosh) (0x1.01f83ep+0) got 0x1.fbc7fap-4
				      want 0x1.fbc7f4p-4.  */
svfloat32_t SV_NAME_F1 (acosh) (svfloat32_t x, const svbool_t pg)
{
  svuint32_t ix = svreinterpret_u32_f32 (x);
  svbool_t special = svcmpge_n_u32 (pg, svsub_n_u32_x (pg, ix, One), Thres);

  svfloat32_t xm1 = svsub_n_f32_x (pg, x, 1);
  svfloat32_t u = svmul_f32_x (pg, xm1, svadd_n_f32_x (pg, x, 1.0f));

  svfloat32_t y
    = sv_log1pf_inline (svadd_f32_x (pg, xm1, svsqrt_f32_x (pg, u)), pg);

  if (unlikely (svptest_any (pg, special)))
    return special_case (x, y, special);
  return y;
}

PL_SIG (SV, F, 1, acosh, 1.0, 10.0)
PL_TEST_ULP (SV_NAME_F1 (acosh), 2.58)
PL_TEST_INTERVAL (SV_NAME_F1 (acosh), 0, 1, 500)
PL_TEST_INTERVAL (SV_NAME_F1 (acosh), 1, SquareLim, 100000)
PL_TEST_INTERVAL (SV_NAME_F1 (acosh), SquareLim, inf, 1000)
PL_TEST_INTERVAL (SV_NAME_F1 (acosh), -0, -inf, 1000)

#endif
