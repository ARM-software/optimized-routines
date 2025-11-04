/*
 * Single-precision SVE modff(x, *y) function (structure-return).
 *
 * Copyright (c) 2025, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "sv_math.h"
#include "sv_modff_inline.h"
#include "test_sig.h"
#include "test_defs.h"

/* Modff algorithm. Produces exact values in all rounding modes.  */
svfloat32x2_t SV_NAME_F1_STRET (modf) (svfloat32_t x, svbool_t pg)
{
  return sv_modff_inline (pg, x);
}

TEST_ULP (_ZGVsMxv_modff_stret_frac, 0.0)
TEST_SYM_INTERVAL (_ZGVsMxv_modff_stret_frac, 0, 1, 20000)
TEST_SYM_INTERVAL (_ZGVsMxv_modff_stret_frac, 1, inf, 20000)

TEST_ULP (_ZGVsMxv_modff_stret_int, 0.0)
TEST_SYM_INTERVAL (_ZGVsMxv_modff_stret_int, 0, 1, 20000)
TEST_SYM_INTERVAL (_ZGVsMxv_modff_stret_int, 1, inf, 20000)
CLOSE_SVE_ATTR
