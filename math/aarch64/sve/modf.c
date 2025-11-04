/*
 * Double-precision SVE modf(x, *y) function.
 *
 * Copyright (c) 2024-2025, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "sv_math.h"
#include "sv_modf_inline.h"
#include "test_sig.h"
#include "test_defs.h"

/* Modf algorithm. Produces exact values in all rounding modes.  */
svfloat64_t SV_NAME_D1_L1 (modf) (svfloat64_t x, double *out_int,
				  const svbool_t pg)
{
  svfloat64x2_t res = sv_modf_inline (pg, x);
  svst1_f64 (pg, out_int, svget2 (res, 1));
  return svget2 (res, 0);
}

TEST_ULP (_ZGVsMxvl8_modf_frac, 0.0)
TEST_SYM_INTERVAL (_ZGVsMxvl8_modf_frac, 0, 1, 20000)
TEST_SYM_INTERVAL (_ZGVsMxvl8_modf_frac, 1, inf, 20000)

TEST_ULP (_ZGVsMxvl8_modf_int, 0.0)
TEST_SYM_INTERVAL (_ZGVsMxvl8_modf_int, 0, 1, 20000)
TEST_SYM_INTERVAL (_ZGVsMxvl8_modf_int, 1, inf, 20000)
CLOSE_SVE_ATTR
