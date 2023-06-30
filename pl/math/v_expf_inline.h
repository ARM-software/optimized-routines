/*
 * Helper for single-precision routines which calculate exp(x) and do not
 * need special-case handling
 *
 * Copyright (c) 2019-2023, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#ifndef PL_MATH_V_EXPF_INLINE_H
#define PL_MATH_V_EXPF_INLINE_H

#include "v_math.h"

static const struct data
{
  float32x4_t poly[5];
  float32x4_t shift, invln2, ln2_hi, ln2_lo;
} data = {
  /* maxerr: 1.45358 +0.5 ulp.  */
  .poly = { V4 (0x1.0e4020p-7f), V4 (0x1.573e2ep-5f), V4 (0x1.555e66p-3f),
	    V4 (0x1.fffdb6p-2f), V4 (0x1.ffffecp-1f) },
  .shift = V4 (0x1.8p23f),
  .invln2 = V4 (0x1.715476p+0f),
  .ln2_hi = V4 (0x1.62e4p-1f),
  .ln2_lo = V4 (0x1.7f7d1cp-20f),
};

#define ExponentBias v_u32 (0x3f800000) /* asuint(1.0f).  */
#define C(i) d->poly[i]

static inline float32x4_t
v_expf_inline (float32x4_t x)
{
  const struct data *d = ptr_barrier (&data);
  /* Helper routine for calculating exp(x).
     Copied from v_expf.c, with all special-case handling removed - the
     calling routine should handle special values if required.  */

  /* exp(x) = 2^n (1 + poly(r)), with 1 + poly(r) in [1/sqrt(2),sqrt(2)]
     x = ln2*n + r, with r in [-ln2/2, ln2/2].  */
  float32x4_t n, r, z;
  z = vfmaq_f32 (d->shift, x, d->invln2);
  n = vsubq_f32 (z, d->shift);
  r = vfmsq_f32 (x, n, d->ln2_hi);
  r = vfmsq_f32 (r, n, d->ln2_lo);
  uint32x4_t e = vshlq_n_u32 (vreinterpretq_u32_f32 (z), 23);
  float32x4_t scale = vreinterpretq_f32_u32 (vaddq_u32 (e, ExponentBias));

  /* Custom order-4 Estrin avoids building high order monomial.  */
  float32x4_t r2 = vmulq_f32 (r, r);
  float32x4_t p, q, poly;
  p = vfmaq_f32 (C (1), C (0), r);
  q = vfmaq_f32 (C (3), C (2), r);
  q = vfmaq_f32 (q, p, r2);
  p = vmulq_f32 (C (4), r);
  poly = vfmaq_f32 (p, q, r2);
  return vfmaq_f32 (scale, poly, scale);
}

#endif // PL_MATH_V_EXPF_INLINE_H
