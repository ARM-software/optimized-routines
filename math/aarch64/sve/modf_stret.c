/*
 * Double-precision SVE modf(x, *y) function (structure-return).
 *
 * Copyright (c) 2025, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "sv_math.h"
#include "sv_modf_inline.h"
#include "test_sig.h"
#include "test_defs.h"

/* Modf algorithm. Produces exact values in all rounding modes.  */
svfloat64x2_t SV_NAME_D1_STRET (modf) (svfloat64_t x, svbool_t pg)
{
  return sv_modf_inline (pg, x);
}

TEST_ULP (_ZGVsMxv_modf_stret_frac, 0.0)
TEST_SYM_INTERVAL (_ZGVsMxv_modf_stret_frac, 0, 1, 20000)
TEST_SYM_INTERVAL (_ZGVsMxv_modf_stret_frac, 1, inf, 20000)

TEST_ULP (_ZGVsMxv_modf_stret_int, 0.0)
TEST_SYM_INTERVAL (_ZGVsMxv_modf_stret_int, 0, 1, 20000)
TEST_SYM_INTERVAL (_ZGVsMxv_modf_stret_int, 1, inf, 20000)
CLOSE_SVE_ATTR
