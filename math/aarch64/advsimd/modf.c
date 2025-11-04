/*
 * Double-precision vector modf(x, *y) function.
 *
 * Copyright (c) 2024-2025, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "v_math.h"
#include "v_modf_inline.h"
#include "test_sig.h"
#include "test_defs.h"

/* Modf algorithm. Produces exact values in all rounding modes.  */
float64x2_t VPCS_ATTR V_NAME_D1_L1 (modf) (float64x2_t x, double *out_int)
{
  float64x2x2_t res = v_modf_inline (x);
  vst1q_f64 (out_int, res.val[1]);
  return res.val[0];
}
TEST_ULP (_ZGVnN2vl8_modf_frac, 0.0)
TEST_SYM_INTERVAL (_ZGVnN2vl8_modf_frac, 0, 1, 20000)
TEST_SYM_INTERVAL (_ZGVnN2vl8_modf_frac, 1, inf, 20000)

TEST_ULP (_ZGVnN2vl8_modf_int, 0.0)
TEST_SYM_INTERVAL (_ZGVnN2vl8_modf_int, 0, 1, 20000)
TEST_SYM_INTERVAL (_ZGVnN2vl8_modf_int, 1, inf, 20000)

