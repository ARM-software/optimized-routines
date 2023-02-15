/*
 * Single-precision SVE special cas function for expf routines
 *
 * Copyright (c) 2019-2023, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */
#include "math_config.h"
#include "sv_math.h"

#if SV_SUPPORTED

#define ScaleThres (192.0f)

/* Special-case handler adapted from Neon variant for expf/exp2f.
   Uses s, y and n to produce the final result (normal cases included).
   It performs an update of all lanes!
   Therefore:
   - all previous computation need to be done on all lanes indicated by input
     pg
   - we cannot simply apply the special case to the special-case-activated
     lanes. */
static inline svfloat32_t
__sv_expf_specialcase (svbool_t pg, svfloat32_t poly, svfloat32_t n,
		       svuint32_t e, svbool_t p_cmp1, svfloat32_t scale)
{
  /* s=2^(n/N) may overflow, break it up into s=s1*s2,
     such that exp = s + s*y can be computed as s1*(s2+s2*y)
     and s1*s1 overflows only if n>0.  */

  /* If n<=0 then set b to 0x820...0, 0 otherwise.  */
  svbool_t p_sign = svcmple_n_f32 (pg, n, 0.0f); /* n <= 0.  */
  svuint32_t b
    = svdup_n_u32_z (p_sign, 0x82000000); /* Inactive lanes set to 0.  */

  /* Set s1 to generate overflow depending on sign of exponent n.  */
  svfloat32_t s1
    = sv_as_f32_u32 (svadd_n_u32_x (pg, b, 0x7f000000)); /* b + 0x7f000000.  */
  /* Offset s to avoid overflow in final result if n is below threshold.  */
  svfloat32_t s2 = sv_as_f32_u32 (
    svsub_u32_x (pg, e, b)); /* as_u32 (s) - 0x3010...0 + b.  */

  /* |n| > 192 => 2^(n/N) overflows.  */
  svbool_t p_cmp2 = svacgt_n_f32 (pg, n, ScaleThres);

  svfloat32_t r2 = svmul_f32_x (pg, s1, s1);
  svfloat32_t r1 = svmla_f32_x (pg, s2, s2, poly);
  r1 = svmul_f32_x (pg, r1, s1);
  svfloat32_t r0 = svmla_f32_x (pg, scale, scale, poly);

  /* Apply condition 1 then 2.
     Returns r2 if cond2 is true, otherwise
     if cond1 is true then return r1, otherwise return r0.  */
  svfloat32_t r = svsel_f32 (p_cmp1, r1, r0);

  return svsel_f32 (p_cmp2, r2, r);
}

#endif
