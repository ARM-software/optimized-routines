/*
 * Half-precision vector e^x function.
 *
 * Copyright (c) 2026, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "mathlib.h"
#include "sv_math.h"
#include "test_sig.h"
#include "test_defs.h"

/* Vector version of expf16.
   The maximum observed error is 0.00 + 0.5 ULP.
   _ZGVsMxv_expf16 (0x1.73cp-6)
    got 0x1.060p+0
   want 0x1.05cp+0.  */
svfloat16_t SV_NAME_H1 (exp) (svfloat16_t x, svbool_t pg)
{
  svfloat16_t x_lo = svreinterpret_f16 (svunpklo (svreinterpret_u16 (x)));
  svfloat16_t x_hi = svreinterpret_f16 (svunpkhi (svreinterpret_u16 (x)));

  svfloat32_t x_lo_f32 = svcvt_f32_x (svptrue_b32 (), x_lo);
  svfloat32_t x_hi_f32 = svcvt_f32_x (svptrue_b32 (), x_hi);

  svfloat32_t exp_lo_f32 = _ZGVsMxv_expf (x_lo_f32, svptrue_b16 ());
  svfloat32_t exp_hi_f32 = _ZGVsMxv_expf (x_hi_f32, svptrue_b16 ());

  svfloat16_t exp_lo = svcvt_f16_x (svptrue_b16 (), exp_lo_f32);
  svfloat16_t exp_hi = svcvt_f16_x (svptrue_b16 (), exp_hi_f32);

  return svuzp1 (exp_lo, exp_hi);
}

TEST_SIG (SV, H, 1, exp, -9.9, 9.9)
TEST_ULP (SV_NAME_H1 (exp), 0.01)
TEST_SYM_INTERVAL (SV_NAME_H1 (exp), 0, 0x1p-31, 5000)
TEST_SYM_INTERVAL (SV_NAME_H1 (exp), 0x1p-31, 0.5, 10000)
TEST_SYM_INTERVAL (SV_NAME_H1 (exp), 0.5, 0x1p31f, 10000)
TEST_SYM_INTERVAL (SV_NAME_H1 (exp), 0x1p31f, inf, 10000)
CLOSE_SVE_ATTR
