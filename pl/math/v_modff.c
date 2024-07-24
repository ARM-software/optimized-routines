/*
 * Single-precision vector modf(x, *y) function.
 *
 * Copyright (c) 2024, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "v_math.h"
#include "pl_sig.h"
#include "pl_test.h"

/* Modff algorithm. Produces exact values in all rounding modes.  */
float32x4_t VPCS_ATTR V_NAME_F1_L1 (modf) (float32x4_t x, float32x4_t *out_int)
{
  uint32x4_t ix = vreinterpretq_u32_f32 (x);
  uint32x4_t ix_abs = vreinterpretq_u32_f32 (vabsq_f32 (x));
  uint32x4_t ix_sign = veorq_u32 (ix, ix_abs);

  /* Extract negative exponent from input.  */
  int32x4_t neg_exp = vsubq_s32 (
      v_s32 (0x7f), vreinterpretq_s32_u32 (vshrq_n_u32 (ix_abs, 23)));

  /* Shift significand mask left by negative exponent.  */
  uint32x4_t frac_mask
      = vreinterpretq_u32_s32 (vshlq_s32 (v_s32 (0x007fffff), neg_exp));

  /* Mask fractional component of float.  */
  uint32x4_t int_component = vbicq_u32 (ix, frac_mask);

  /* If |x| >= 1, out_int = non-fractional part, otherwise +/-0.  */
  *out_int = vreinterpretq_f32_u32 (
      vbslq_u32 (vclezq_s32 (neg_exp), int_component, ix_sign));

  /* Subtract integral component from input.  */
  uint32x4_t remaining = vreinterpretq_u32_f32 (vsubq_f32 (x, *out_int));

  /* Return +/-0 for integral x.  */
  uint32x4_t is_equal = vceqq_f32 (x, *out_int);
  return vreinterpretq_f32_u32 (vbslq_u32 (is_equal, ix_sign, remaining));
}

PL_TEST_ULP (_ZGVnN4vl4_modff_frac, 0.0)
PL_TEST_SYM_INTERVAL (_ZGVnN4vl4_modff_frac, 0, 1, 20000)
PL_TEST_SYM_INTERVAL (_ZGVnN4vl4_modff_frac, 1, inf, 20000)

PL_TEST_ULP (_ZGVnN4vl4_modff_int, 0.0)
PL_TEST_SYM_INTERVAL (_ZGVnN4vl4_modff_int, 0, 1, 20000)
PL_TEST_SYM_INTERVAL (_ZGVnN4vl4_modff_int, 1, inf, 20000)
