/*
 * Single-precision vector modf(x, *y) function.
 *
 * Copyright (c) 2024-2025, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "v_math.h"
#include "v_modff_inline.h"
#include "test_sig.h"
#include "test_defs.h"

/* Modff algorithm. Produces exact values in all rounding modes.  */
float32x4_t VPCS_ATTR NOINLINE V_NAME_F1_L1 (modf) (float32x4_t x,
						    float *out_int)
{
  float32x4x2_t res = v_modff_inline (x);
  vst1q_f32 (out_int, res.val[1]);
  return res.val[0];
}

float32x4x2_t VPCS_ATTR NOINLINE V_NAME_F1_STRET (modf) (float32x4_t x)
{
  return v_modff_inline (x);
}

TEST_ULP (_ZGVnN4vl4_modff_frac, 0.0)
TEST_SYM_INTERVAL (_ZGVnN4vl4_modff_frac, 0, 1, 20000)
TEST_SYM_INTERVAL (_ZGVnN4vl4_modff_frac, 1, inf, 20000)

TEST_ULP (_ZGVnN4vl4_modff_int, 0.0)
TEST_SYM_INTERVAL (_ZGVnN4vl4_modff_int, 0, 1, 20000)
TEST_SYM_INTERVAL (_ZGVnN4vl4_modff_int, 1, inf, 20000)

TEST_ULP (_ZGVnN4v_modff_stret_frac, 0.0)
TEST_SYM_INTERVAL (_ZGVnN4v_modff_stret_frac, 0, 1, 20000)
TEST_SYM_INTERVAL (_ZGVnN4v_modff_stret_frac, 1, inf, 20000)

TEST_ULP (_ZGVnN4v_modff_stret_int, 0.0)
TEST_SYM_INTERVAL (_ZGVnN4v_modff_stret_int, 0, 1, 20000)
TEST_SYM_INTERVAL (_ZGVnN4v_modff_stret_int, 1, inf, 20000)
