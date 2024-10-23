/*
 * Double-precision vector modf(x, *y) function.
 *
 * Copyright (c) 2024, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "v_math.h"
#include "test_sig.h"
#include "test_defs.h"

/* Modf algorithm. Produces exact values in all rounding modes.  */
float64x2_t VPCS_ATTR V_NAME_D1_L1 (modf) (float64x2_t x, float64x2_t *out_int)
{
  /* Get integer component of x.  */
  *out_int = vrndq_f64 (x);

  /* Subtract integer component from input.  */
  uint64x2_t remaining = vreinterpretq_u64_f64 (vsubq_f64 (x, *out_int));

  /* Return +0 for integer x.  */
  uint64x2_t is_integer = vceqq_f64 (x, *out_int);
  return vreinterpretq_f64_u64 (vbicq_u64 (remaining, is_integer));
}

TEST_ULP (_ZGVnN2vl8_modf_frac, 0.0)
TEST_SYM_INTERVAL (_ZGVnN2vl8_modf_frac, 0, 1, 20000)
TEST_SYM_INTERVAL (_ZGVnN2vl8_modf_frac, 1, inf, 20000)

TEST_ULP (_ZGVnN2vl8_modf_int, 0.0)
TEST_SYM_INTERVAL (_ZGVnN2vl8_modf_int, 0, 1, 20000)
TEST_SYM_INTERVAL (_ZGVnN2vl8_modf_int, 1, inf, 20000)
