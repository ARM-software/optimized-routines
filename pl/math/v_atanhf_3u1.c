/*
 * Single-precision vector atanh(x) function.
 * Copyright (c) 2022, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "v_math.h"
#include "mathlib.h"

#if V_SUPPORTED

#define AbsMask 0x7fffffff
#define Half 0x3f000000
#define One 0x3f800000
#define Four 0x40800000
#define Ln2 0x1.62e43p-1f
#define TinyBound 0x39800000 /* 0x1p-12, below which atanhf(x) rounds to x. */

#define C(i) v_f32 (__log1pf_data.coeffs[i])

static inline v_f32_t
eval_poly (v_f32_t m)
{
  /* Approximate log(1+m) on [-0.25, 0.5] using Estrin scheme.  */
  v_f32_t p_12 = v_fma_f32 (m, C (1), C (0));
  v_f32_t p_34 = v_fma_f32 (m, C (3), C (2));
  v_f32_t p_56 = v_fma_f32 (m, C (5), C (4));
  v_f32_t p_78 = v_fma_f32 (m, C (7), C (6));

  v_f32_t m2 = m * m;
  v_f32_t p_02 = v_fma_f32 (m2, p_12, m);
  v_f32_t p_36 = v_fma_f32 (m2, p_56, p_34);
  v_f32_t p_79 = v_fma_f32 (m2, C (8), p_78);

  v_f32_t m4 = m2 * m2;
  v_f32_t p_06 = v_fma_f32 (m4, p_36, p_02);

  return v_fma_f32 (m4, m4 * p_79, p_06);
}

static inline v_f32_t
log1pf_inline (v_f32_t x)
{
  /* Helper for calculating log(x + 1). Copied from log1pf_2u1.c, with no
     special-case handling. See that file for details of the algorithm.  */
  v_f32_t m = x + 1.0f;
  v_u32_t k = (v_as_u32_f32 (m) - 0x3f400000) & 0xff800000;
  v_f32_t s = v_as_f32_u32 (v_u32 (Four) - k);
  v_f32_t m_scale = v_as_f32_u32 (v_as_u32_f32 (x) - k)
		    + v_fma_f32 (v_f32 (0.25f), s, v_f32 (-1.0f));
  v_f32_t p = eval_poly (m_scale);
  v_f32_t scale_back = v_to_f32_u32 (k) * 0x1.0p-23f;
  return v_fma_f32 (scale_back, v_f32 (Ln2), p);
}

/* Approximation for vector single-precision atanh(x) using modified log1p.
   The maximum error is 3.08 ULP:
   __v_atanhf(0x1.ff215p-5) got 0x1.ffcb7cp-5
			   want 0x1.ffcb82p-5.  */
VPCS_ATTR v_f32_t V_NAME (atanhf) (v_f32_t x)
{
  v_u32_t ix = v_as_u32_f32 (x);
  v_f32_t halfsign
    = v_as_f32_u32 (v_bsl_u32 (v_u32 (AbsMask), v_u32 (Half), ix));
  v_u32_t iax = ix & AbsMask;

  v_f32_t ax = v_as_f32_u32 (iax);

#if WANT_ERRNO
  v_u32_t special = v_cond_u32 ((iax >= One) | (iax <= TinyBound));
  /* Side-step special cases by setting those lanes to 0, which will trigger no
     exceptions. These will be fixed up later.  */
  if (unlikely (v_any_u32 (special)))
    ax = v_sel_f32 (special, v_f32 (0), ax);
#else
  v_u32_t special = v_cond_u32 (iax >= One);
#endif

  v_f32_t y = halfsign * log1pf_inline ((2 * ax) / (1 - ax));

  if (unlikely (v_any_u32 (special)))
    return v_call_f32 (atanhf, x, y, special);
  return y;
}

VPCS_ALIAS

#endif
