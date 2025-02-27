/*
 * SVE helper for single-precision routines which calculate exp(x) and do
 * not need special-case handling
 *
 * Copyright (c) 2025, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#ifndef MATH_SV_EXPF_INLINE_H
#define MATH_SV_EXPF_INLINE_H

#include "sv_math.h"
#include "test_sig.h"
#include "test_defs.h"

struct sv_expf_data
{
  float ln2_hi, ln2_lo, inv_ln2, shift;
};

#define SV_EXPF_DATA                                                          \
  {                                                                           \
    /* Shift is 1.5*2^17 + 127.  */                                           \
    .shift = 0x1.803f8p17f, .inv_ln2 = 0x1.715476p+0f,                        \
    .ln2_hi = 0x1.62e4p-1f, .ln2_lo = 0x1.7f7d1cp-20f,                        \
  }

static inline svfloat32_t
expf_inline (svfloat32_t x, const svbool_t pg, const struct sv_expf_data *d)
{
  /* exp(x) = 2^n (1 + poly(r)), with 1 + poly(r) in [1/sqrt(2),sqrt(2)]
     x = ln2*n + r, with r in [-ln2/2, ln2/2].  */

  svfloat32_t lane_consts = svld1rq (svptrue_b32 (), &d->ln2_hi);

  /* n = round(x/(ln2/N)).  */
  svfloat32_t z = svmla_lane (sv_f32 (d->shift), x, lane_consts, 2);
  svfloat32_t n = svsub_x (pg, z, d->shift);

  /* r = x - n*ln2/N.  */
  svfloat32_t r;
  r = svmls_lane (x, n, lane_consts, 0);
  r = svmls_lane (r, n, lane_consts, 1);

  /* scale = 2^(n/N).  */
  svfloat32_t scale = svexpa (svreinterpret_u32 (z));

  /* poly(r) = exp(r) - 1 ~= r.  */
  return svmla_x (pg, scale, scale, r);
}

#endif // MATH_SV_EXPF_INLINE_H
