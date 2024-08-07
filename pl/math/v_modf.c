/*
 * Double-precision vector modf(x, *y) function.
 *
 * Copyright (c) 2024, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "v_math.h"
#include "pl_sig.h"
#include "pl_test.h"

/* Modf algorithm. Produces exact values in all rounding modes.  */
float64x2_t VPCS_ATTR V_NAME_D1_L1 (modf) (float64x2_t x, float64x2_t *out_int)
{
  /* Get sign of x.  */
  uint64x2_t ix_sign = veorq_u64 (vreinterpretq_u64_f64 (x),
				  vreinterpretq_u64_f64 (vabsq_f64 (x)));

  /* Get integer component of x.  */
  *out_int = vrndq_f64 (x);

  /* Subtract integer component from input.  */
  uint64x2_t remaining = vreinterpretq_u64_f64 (vsubq_f64 (x, *out_int));

  /* Return +/-0 for integer x.  */
  uint64x2_t is_integer = vceqq_f64 (x, *out_int);
  return vreinterpretq_f64_u64 (vbslq_u64 (is_integer, ix_sign, remaining));
}

PL_TEST_ULP (_ZGVnN2vl8_modf_frac, 0.0)
PL_TEST_SYM_INTERVAL (_ZGVnN2vl8_modf_frac, 0, 1, 20000)
PL_TEST_SYM_INTERVAL (_ZGVnN2vl8_modf_frac, 1, inf, 20000)

PL_TEST_ULP (_ZGVnN2vl8_modf_int, 0.0)
PL_TEST_SYM_INTERVAL (_ZGVnN2vl8_modf_int, 0, 1, 20000)
PL_TEST_SYM_INTERVAL (_ZGVnN2vl8_modf_int, 1, inf, 20000)
