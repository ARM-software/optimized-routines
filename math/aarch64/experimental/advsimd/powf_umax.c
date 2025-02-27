/*
 * Low accuracy single-precision vector pow(x, y) function.
 *
 * Copyright (c) 2025, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "mathlib.h"
#include "v_math.h"
#include "test_defs.h"

/* Fast inaccurate powf.
   Only tested with make check yet.
   Maximum measured error: 214.066 +0.5 ULP. BASIS 2.
   Maximum measured error: 262.151 +0.5 ULP. BASIS e.
   Maximum measured error: 249.575 +0.5 ULP. BASIS 10.  */
float32x4_t VPCS_ATTR NOINLINE
arm_math_advsimd_fast_powf (float32x4_t x, float32x4_t y)
{
  float32x4_t logx = _ZGVnN4v_log2f (x);
  float32x4_t ylogx = vmulq_f32 (y, logx);
  return _ZGVnN4v_exp2f (ylogx);
}

TEST_ULP (arm_math_advsimd_fast_powf, 4096)
TEST_DISABLE_FENV (arm_math_advsimd_fast_powf)
TEST_INTERVAL2 (arm_math_advsimd_fast_powf, 0x1p-1, 0x1p1, 0x1p-7, 0x1p7,
		500000)
TEST_INTERVAL2 (arm_math_advsimd_fast_powf, 0x1p-1, 0x1p1, -0x1p-7, -0x1p7,
		500000)
TEST_INTERVAL2 (arm_math_advsimd_fast_powf, 0x1p-70, 0x1p70, 0x1p-1, 0x1p1,
		500000)
TEST_INTERVAL2 (arm_math_advsimd_fast_powf, 0x1p-70, 0x1p70, -0x1p-1, -0x1p1,
		500000)
TEST_INTERVAL2 (arm_math_advsimd_fast_powf, 0x1.ep-1, 0x1.1p0, 0x1p8, 0x1p14,
		500000)
TEST_INTERVAL2 (arm_math_advsimd_fast_powf, 0x1.ep-1, 0x1.1p0, -0x1p8, -0x1p14,
		500000)
