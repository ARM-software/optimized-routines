/*
 * Helper for SVE double-precision routines which calculate log(1 + x) and do
 * not need special-case handling
 *
 * Copyright (c) 2022-2023, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */
#ifndef PL_MATH_SV_LOG1P_INLINE_H
#define PL_MATH_SV_LOG1P_INLINE_H

#include "sv_math.h"
#include "sv_pairwise_horner.h"

#if SV_SUPPORTED

#define Ln2Hi 0x1.62e42fefa3800p-1
#define Ln2Lo 0x1.ef35793c76730p-45
#define BottomMask 0xffffffff
#define HfRt2Top 0x3fe6a09e00000000 /* top32(asuint64(sqrt(2)/2)) << 32.  */
#define OneMHfRt2Top                                                           \
  0x00095f6200000000 /* (top32(asuint64(1)) - top32(asuint64(sqrt(2)/2)))      \
			<< 32.  */
#define OneTop 0x3ff
#define C(i) (__log1p_data.coeffs[i])

static inline svfloat64_t
sv_log1p_inline (svfloat64_t x, const svbool_t pg)
{
  /* Helper for calculating log(x + 1). Adapted from v_log1p_inline.h, which
     differs from v_log1p_2u5.c by:
     - No special-case handling - this should be dealt with by the caller.
     - Pairwise Horner polynomial evaluation for improved accuracy.
     - Optionally simulate the shortcut for k=0, used in the scalar routine,
       using svsel, for improved accuracy when the argument to log1p is close to
       0. This feature is enabled by defining WANT_SV_LOG1P_K0_SHORTCUT as 1 in
       the source of the caller before including this file.
     See v_log1pf_2u1.c for details of the algorithm.  */
  svfloat64_t m = svadd_n_f64_x (pg, x, 1);
  svuint64_t mi = svreinterpret_u64_f64 (m);
  svuint64_t u = svadd_n_u64_x (pg, mi, OneMHfRt2Top);

  svint64_t ki
    = svsub_n_s64_x (pg, svreinterpret_s64_u64 (svlsr_n_u64_x (pg, u, 52)),
		     OneTop);
  svfloat64_t k = svcvt_f64_s64_x (pg, ki);

  /* Reduce x to f in [sqrt(2)/2, sqrt(2)].  */
  svuint64_t utop
    = svadd_n_u64_x (pg, svand_n_u64_x (pg, u, 0x000fffff00000000), HfRt2Top);
  svuint64_t u_red = svorr_u64_x (pg, utop, svand_n_u64_x (pg, mi, BottomMask));
  svfloat64_t f = svsub_n_f64_x (pg, svreinterpret_f64_u64 (u_red), 1);

  /* Correction term c/m.  */
  svfloat64_t c = svsub_f64_x (pg, x, svsub_n_f64_x (pg, m, 1));
  svfloat64_t cm;

#ifndef WANT_SV_LOG1P_K0_SHORTCUT
#error                                                                         \
  "Cannot use sv_log1p_inline.h without specifying whether you need the k0 shortcut for greater accuracy close to 0"
#elif WANT_SV_LOG1P_K0_SHORTCUT
  /* Shortcut if k is 0 - set correction term to 0 and f to x. The result is
     that the approximation is solely the polynomial.  */
  svbool_t knot0 = svcmpne_n_f64 (pg, k, 0);
  cm = svdiv_f64_z (knot0, c, m);
  if (likely (!svptest_any (pg, knot0)))
    {
      f = svsel_f64 (knot0, f, x);
    }
#else
  /* No shortcut.  */
  cm = svdiv_f64_x (pg, c, m);
#endif

  /* Approximate log1p(f) on the reduced input using a polynomial.  */
  svfloat64_t f2 = svmul_f64_x (pg, f, f);
  svfloat64_t p = PAIRWISE_HORNER_18 (pg, f, f2, C);

  /* Assemble log1p(x) = k * log2 + log1p(f) + c/m.  */
  svfloat64_t ylo = svmla_n_f64_x (pg, cm, k, Ln2Lo);
  svfloat64_t yhi = svmla_n_f64_x (pg, f, k, Ln2Hi);
  return svmla_f64_x (pg, svadd_f64_x (pg, ylo, yhi), f2, p);
}
#endif
#endif // PL_MATH_SV_LOG1P_INLINE_H
