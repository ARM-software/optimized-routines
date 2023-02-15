/*
 * Helper for vector double-precision routines which calculate log(1 + x) and do
 * not need special-case handling
 *
 * Copyright (c) 2022-2023, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */
#ifndef PL_MATH_V_LOG1P_INLINE_H
#define PL_MATH_V_LOG1P_INLINE_H

#include "v_math.h"
#include "pairwise_horner.h"

#define Ln2Hi v_f64 (0x1.62e42fefa3800p-1)
#define Ln2Lo v_f64 (0x1.ef35793c76730p-45)
#define HfRt2Top 0x3fe6a09e00000000 /* top32(asuint64(sqrt(2)/2)) << 32.  */
#define OneMHfRt2Top                                                           \
  0x00095f6200000000 /* (top32(asuint64(1)) - top32(asuint64(sqrt(2)/2)))      \
			<< 32.  */
#define OneTop 0x3ff
#define BottomMask 0xffffffff
#define BigBoundTop 0x5fe /* top12 (asuint64 (0x1p511)).  */

#define C(i) v_f64 (__log1p_data.coeffs[i])

static inline float64x2_t
log1p_inline (float64x2_t x)
{
  /* Helper for calculating log(x + 1). Copied from v_log1p_2u5.c, with several
     modifications:
     - No special-case handling - this should be dealt with by the caller.
     - Pairwise Horner polynomial evaluation for improved accuracy.
     - Optionally simulate the shortcut for k=0, used in the scalar routine,
       using v_sel, for improved accuracy when the argument to log1p is close to
       0. This feature is enabled by defining WANT_V_LOG1P_K0_SHORTCUT as 1 in
       the source of the caller before including this file.
     See v_log1pf_2u1.c for details of the algorithm.  */
  float64x2_t m = x + 1;
  uint64x2_t mi = v_as_u64_f64 (m);
  uint64x2_t u = mi + OneMHfRt2Top;

  int64x2_t ki = v_as_s64_u64 (u >> 52) - OneTop;
  float64x2_t k = vcvtq_f64_s64 (ki);

  /* Reduce x to f in [sqrt(2)/2, sqrt(2)].  */
  uint64x2_t utop = (u & 0x000fffff00000000) + HfRt2Top;
  uint64x2_t u_red = utop | (mi & BottomMask);
  float64x2_t f = v_as_f64_u64 (u_red) - 1;

  /* Correction term c/m.  */
  float64x2_t cm = (x - (m - 1)) / m;

#ifndef WANT_V_LOG1P_K0_SHORTCUT
#error                                                                         \
  "Cannot use v_log1p_inline.h without specifying whether you need the k0 shortcut for greater accuracy close to 0"
#elif WANT_V_LOG1P_K0_SHORTCUT
  /* Shortcut if k is 0 - set correction term to 0 and f to x. The result is
     that the approximation is solely the polynomial. */
  uint64x2_t k0 = k == 0;
  if (unlikely (v_any_u64 (k0)))
    {
      cm = vbslq_f64 (k0, v_f64 (0), cm);
      f = vbslq_f64 (k0, x, f);
    }
#endif

  /* Approximate log1p(f) on the reduced input using a polynomial.  */
  float64x2_t f2 = f * f;
  float64x2_t p = PAIRWISE_HORNER_18 (f, f2, C);

  /* Assemble log1p(x) = k * log2 + log1p(f) + c/m.  */
  float64x2_t ylo = vfmaq_f64 (cm, k, Ln2Lo);
  float64x2_t yhi = vfmaq_f64 (f, k, Ln2Hi);
  return vfmaq_f64 (ylo + yhi, f2, p);
}

#endif // PL_MATH_V_LOG1P_INLINE_H
