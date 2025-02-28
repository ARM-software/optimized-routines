/*
 * SVE helper for single-precision routines which calculate exp(x) and do
 * not need special-case handling
 *
 * Copyright (c) 2023-2025, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#ifndef MATH_SV_EXPF_INLINE_H
#define MATH_SV_EXPF_INLINE_H

#include "sv_math.h"
#include "test_sig.h"
#include "test_defs.h"

struct sv_expf_data
{
  float ln2_hi, ln2_lo, c1, null;
  float inv_ln2, shift;
};

/* Shift is 1.5*2^17 + 127.  */
#define SV_EXPF_DATA                                                          \
  {                                                                           \
    .c1 = 0.5f, .inv_ln2 = 0x1.715476p+0f, .ln2_hi = 0x1.62e4p-1f,            \
    .ln2_lo = 0x1.7f7d1cp-20f, .shift = 0x1.803f8p17f,                        \
  }

static inline svfloat32_t
expf_inline (svfloat32_t x, const svbool_t pg, const struct sv_expf_data *d)
{
  /* exp(x) = 2^n (1 + poly(r)), with 1 + poly(r) in [1/sqrt(2),sqrt(2)]
     x = ln2*n + r, with r in [-ln2/2, ln2/2].  */

  svfloat32_t lane_consts = svld1rq (svptrue_b32 (), &d->ln2_hi);

  /* n = round(x/(ln2/N)).  */
  svfloat32_t z = svmad_x (pg, sv_f32 (d->inv_ln2), x, d->shift);
  svfloat32_t n = svsub_x (pg, z, d->shift);

  /* r = x - n*ln2/N.  */
  svfloat32_t r = x;
  r = svmls_lane (r, n, lane_consts, 0);
  r = svmls_lane (r, n, lane_consts, 1);

  /* scale = 2^(n/N).  */
  svfloat32_t scale = svexpa (svreinterpret_u32 (z));

  /* poly(r) = exp(r) - 1 ~= r + 0.5 r^2.  */
  svfloat32_t r2 = svmul_x (svptrue_b32 (), r, r);
  svfloat32_t poly = svmla_lane (r, r2, lane_consts, 2);

  return svmla_x (pg, scale, scale, poly);
}

#endif // MATH_SV_EXPF_INLINE_H
