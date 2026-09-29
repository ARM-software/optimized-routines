/*
 * Half-precision vector e^x function.
 *
 * Copyright (c) 2026, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "mathlib.h"
#include "v_math.h"
#include "test_sig.h"
#include "test_defs.h"

/* Vector version of expf16.
   The maximum observed error is 0.00 + 0.5 ULP.
   _ZGVnN8v_expf16 (-0x1.4ecp+1)
    got 0x1.2b8p-4
   want 0x1.2bcp-4.  */
float16x8_t VPCS_ATTR FP16_ATTR NOINLINE V_NAME_H1 (exp) (float16x8_t x)
{
  float16x4_t x_lo = vget_low_f16 (x);
  float16x4_t x_hi = vget_high_f16 (x);

  float32x4_t x_lo_f32 = vcvt_f32_f16 (x_lo);
  float32x4_t x_hi_f32 = vcvt_f32_f16 (x_hi);

  float32x4_t res_lo_f32 = _ZGVnN4v_expf (x_lo_f32);
  float32x4_t res_hi_f32 = _ZGVnN4v_expf (x_hi_f32);

  float16x4_t res_lo = vcvt_f16_f32 (res_lo_f32);
  float16x4_t res_hi = vcvt_f16_f32 (res_hi_f32);

  return vcombine_f16 (res_lo, res_hi);
}

HALF_WIDTH_ALIAS_H1 (exp)

TEST_SIG (V, H, 1, exp, -9.9, 9.9)
TEST_ULP (V_NAME_H1 (exp), 0.01)
TEST_SYM_INTERVAL (V_NAME_H1 (exp), 0, 0x1p-31, 5000)
TEST_SYM_INTERVAL (V_NAME_H1 (exp), 0x1p-31, 0.5, 10000)
TEST_SYM_INTERVAL (V_NAME_H1 (exp), 0.5, 0x1p31f, 10000)
TEST_SYM_INTERVAL (V_NAME_H1 (exp), 0x1p31f, inf, 10000)
