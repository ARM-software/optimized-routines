/*
 * Single-precision SVE asinh(x) function.
 *
 * Copyright (c) 2023, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "sv_math.h"
#include "include/mathlib.h"
#include "pl_sig.h"
#include "pl_test.h"

#if SV_SUPPORTED

#include "sv_log1pf_inline.h"

#define SignMask (0x80000000)
#define One (1.0f)
#define BigBound (0x5f800000)  /* asuint(0x1p64).  */
#define TinyBound (0x30800000) /* asuint(0x1p-30).  */

static NOINLINE svfloat32_t
special_case (svfloat32_t x, svfloat32_t y, svbool_t special, svbool_t pg)
{
  return sv_call_f32 (asinhf, x, y, special);
}

/* Single-precision SVE asinh(x) routine. Implements the same algorithm as
   vector asinhf and log1p.

   Maximum error is 2.76 ULPs:
   SV_NAME_F1 (asinh) (0x1.0251b4p-2) got 0x1.ff4f52p-3
	  want 0x1.ff4f58p-3  */
svfloat32_t SV_NAME_F1 (asinh) (svfloat32_t x, const svbool_t pg)
{
  svuint32_t ix = svreinterpret_u32_f32 (x);
  svuint32_t iax = svbic_n_u32_x (pg, ix, SignMask);
  svuint32_t sign = svand_n_u32_x (pg, ix, SignMask);
  svfloat32_t ax = svreinterpret_f32_u32 (iax);
  svbool_t special = svcmpge_n_u32 (pg, iax, BigBound);

  /* asinh(x) = log(x + sqrt(x * x + 1)).
     For positive x, asinh(x) = log1p(x + x * x / (1 + sqrt(x * x + 1))).  */
  svfloat32_t ax2 = svmul_f32_x (pg, ax, ax);
  svfloat32_t d
    = svadd_n_f32_x (pg, svsqrt_f32_x (pg, svadd_n_f32_x (pg, ax2, One)), One);
  svfloat32_t y
    = sv_log1pf_inline (svadd_f32_x (pg, ax, svdiv_f32_x (pg, ax2, d)), pg);
  y = svreinterpret_f32_u32 (svorr_u32_x (pg, sign, svreinterpret_u32_f32 (y)));

  if (unlikely (svptest_any (pg, special)))
    return special_case (x, y, special, pg);
  return y;
}

PL_SIG (SV, F, 1, asinh, -10.0, 10.0)
PL_TEST_ULP (SV_NAME_F1 (asinh), 2.17)
PL_TEST_INTERVAL (SV_NAME_F1 (asinh), 0, 0x1p-12, 40000)
PL_TEST_INTERVAL (SV_NAME_F1 (asinh), 0x1p-12, 1.0, 40000)
PL_TEST_INTERVAL (SV_NAME_F1 (asinh), 1.0, 0x1p11, 40000)
PL_TEST_INTERVAL (SV_NAME_F1 (asinh), 0x1p11, inf, 40000)
PL_TEST_INTERVAL (SV_NAME_F1 (asinh), 0, -0x1p-12, 20000)
PL_TEST_INTERVAL (SV_NAME_F1 (asinh), -0x1p-12, -1.0, 20000)
PL_TEST_INTERVAL (SV_NAME_F1 (asinh), -1.0, -0x1p11, 20000)
PL_TEST_INTERVAL (SV_NAME_F1 (asinh), -0x1p11, -inf, 20000)

#endif