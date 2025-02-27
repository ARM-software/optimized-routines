/*
 * Low-accuracy single-precision SVE pow function.
 *
 * Copyright (c) 2025, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "mathlib.h"
#include "sv_math.h"
#include "test_defs.h"

/* Fast inaccurate SVE powf.
   Only tested with make check yet.
   Testing special cases like x < 0 would break if tested,
   we currently do not test these cases,
   but ideally they should just be allowed to break.
   Maximum measured error: 215.448 +0.5 ULP. BASIS 2.  */
svfloat32_t
arm_math_sve_fast_powf (svfloat32_t x, svfloat32_t y, const svbool_t pg)
{
  svfloat32_t logx = _ZGVsMxv_log2f (x, pg);
  svfloat32_t ylogx = svmul_x (svptrue_b32 (), y, logx);
  return _ZGVsMxv_exp2f (ylogx, pg);
}

TEST_ULP (arm_math_sve_fast_powf, 4096)
TEST_DISABLE_FENV (arm_math_sve_fast_powf)
/* Wide intervals spanning the whole domain but shared between x and y.  */
#define SV_POWF_INTERVAL2(xlo, xhi, ylo, yhi, n)                              \
  TEST_INTERVAL2 (arm_math_sve_fast_powf, xlo, xhi, ylo, yhi, n)              \
  TEST_INTERVAL2 (arm_math_sve_fast_powf, xlo, xhi, -ylo, -yhi, n)
SV_POWF_INTERVAL2 (0, 0x1p-126, 0, inf, 40000)
SV_POWF_INTERVAL2 (0x1p-126, 1, 0, inf, 50000)
SV_POWF_INTERVAL2 (1, inf, 0, inf, 50000)
/* x~1 or y~1.  */
SV_POWF_INTERVAL2 (0x1p-1, 0x1p1, 0x1p-10, 0x1p10, 10000)
SV_POWF_INTERVAL2 (0x1.ep-1, 0x1.1p0, 0x1p8, 0x1p16, 10000)
SV_POWF_INTERVAL2 (0x1p-500, 0x1p500, 0x1p-1, 0x1p1, 10000)
/* around estimated argmaxs of ULP error.  */
SV_POWF_INTERVAL2 (0x1p-300, 0x1p-200, 0x1p-20, 0x1p-10, 10000)
SV_POWF_INTERVAL2 (0x1p50, 0x1p100, 0x1p-20, 0x1p-10, 10000)
CLOSE_SVE_ATTR
