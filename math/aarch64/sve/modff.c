/*
 * Single-precision SVE modff(x, *y) function.
 *
 * Copyright (c) 2024-2025, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "sv_math.h"
#include "sv_modff_inline.h"
#include "test_sig.h"
#include "test_defs.h"

/* Modff algorithm. Produces exact values in all rounding modes.  */
svfloat32_t SV_NAME_F1_L1 (modf) (svfloat32_t x, float *out_int,
				  const svbool_t pg)
{
  svfloat32x2_t res = sv_modff_inline (pg, x);
  svst1_f32 (pg, out_int, svget2 (res, 1));
  return svget2 (res, 0);
}

TEST_ULP (_ZGVsMxvl4_modff_frac, 0.0)
TEST_SYM_INTERVAL (_ZGVsMxvl4_modff_frac, 0, 1, 20000)
TEST_SYM_INTERVAL (_ZGVsMxvl4_modff_frac, 1, inf, 20000)

TEST_ULP (_ZGVsMxvl4_modff_int, 0.0)
TEST_SYM_INTERVAL (_ZGVsMxvl4_modff_int, 0, 1, 20000)
TEST_SYM_INTERVAL (_ZGVsMxvl4_modff_int, 1, inf, 20000)
CLOSE_SVE_ATTR
