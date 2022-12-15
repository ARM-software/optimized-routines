/*
 * Single-precision vector tanh(x) function.
 * Copyright (c) 2022, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "v_math.h"
#include "estrinf.h"
#include "mathlib.h"
#include "pl_sig.h"
#include "pl_test.h"

#if V_SUPPORTED

#define BoringBound                                                            \
  0x41102cb3 /* 0x1.205966p+3, above which tanhf rounds to 1 (or -1 for        \
		negative).  */
#define AbsMask 0x7fffffff
#define One 0x3f800000

#define Shift v_f32 (0x1.8p23f)
#define InvLn2 v_f32 (0x1.715476p+0f)
#define MLn2hi v_f32 (-0x1.62e4p-1f)
#define MLn2lo v_f32 (-0x1.7f7d1cp-20f)

#define C(i) v_f32 (__expm1f_poly[i])

static inline v_f32_t
expm1f_inline (v_f32_t x)
{
  /* Helper routine for calculating exp(x) - 1.
     Copied from v_expm1f_1u6.c, with all special-case handling removed, as
     special, tiny and large values are all dealt with in the main tanhf
     routine.  */

  /* Reduce argument: f in [-ln2/2, ln2/2], i is exact.  */
  v_f32_t j = v_fma_f32 (InvLn2, x, Shift) - Shift;
  v_s32_t i = v_to_s32_f32 (j);
  v_f32_t f = v_fma_f32 (j, MLn2hi, x);
  f = v_fma_f32 (j, MLn2lo, f);

  /* Approximate expm1(f) with polynomial P, expm1(f) ~= f + f^2 * P(f).
     Uses Estrin scheme, where the main __v_expm1f routine uses Horner.  */
  v_f32_t f2 = f * f;
  v_f32_t p = ESTRIN_4 (f, f2, f2 * f2, C);
  p = v_fma_f32 (f2, p, f);

  /* t = 2^i.  */
  v_f32_t t = v_as_f32_u32 (v_as_u32_s32 (i << 23) + One);
  /* expm1(x) ~= p * t + (t - 1).  */
  return v_fma_f32 (p, t, t - 1);
}

static NOINLINE v_f32_t
special_case (v_f32_t x, v_f32_t y, v_u32_t special)
{
  return v_call_f32 (tanhf, x, y, special);
}

/* Approximation for single-precision vector tanh(x), using a simplified version
   of expm1f. The maximum error is 2.58 ULP:
   __v_tanhf(0x1.fa5eep-5) got 0x1.f9ba02p-5
			  want 0x1.f9ba08p-5.  */
VPCS_ATTR v_f32_t V_NAME (tanhf) (v_f32_t x)
{
  v_u32_t ix = v_as_u32_f32 (x);
  v_u32_t iax = ix & AbsMask;
  v_u32_t sign = ix & ~AbsMask;
  v_u32_t is_boring = v_cond_u32 (iax > BoringBound);
  v_f32_t boring = v_as_f32_u32 (sign | One);

#if WANT_ERRNO
  /* If errno needs to be set properly, set all special and boring lanes to 1,
     which will trigger no exceptions, and fix them up later.  */
  v_u32_t special = v_cond_u32 ((iax > 0x7f800000) | (iax < 0x34000000));
  ix = v_sel_u32 (is_boring, v_u32 (One), ix);
  if (unlikely (v_any_u32 (special)))
    ix = v_sel_u32 (special, v_u32 (One), ix);
#else
  v_u32_t special = v_cond_u32 ((iax > 0x7f800000) | (iax == 0));
#endif

  /* tanh(x) = (e^2x - 1) / (e^2x + 1).  */
  v_f32_t q = expm1f_inline (2 * v_as_f32_u32 (ix));
  v_f32_t y = q / (q + 2);
  y = v_sel_f32 (is_boring, boring, y);
  if (unlikely (v_any_u32 (special)))
    return special_case (x, y, special);
  return y;
}
VPCS_ALIAS

PL_SIG (V, F, 1, tanh, -10.0, 10.0)
PL_TEST_ULP (V_NAME (tanhf), 2.09)
PL_TEST_EXPECT_FENV (V_NAME (tanhf), WANT_ERRNO)
PL_TEST_INTERVAL (V_NAME (tanhf), 0, 0x1p-23, 1000)
PL_TEST_INTERVAL (V_NAME (tanhf), -0, -0x1p-23, 1000)
PL_TEST_INTERVAL (V_NAME (tanhf), 0x1p-23, 0x1.205966p+3, 100000)
PL_TEST_INTERVAL (V_NAME (tanhf), -0x1p-23, -0x1.205966p+3, 100000)
PL_TEST_INTERVAL (V_NAME (tanhf), 0x1.205966p+3, inf, 100)
PL_TEST_INTERVAL (V_NAME (tanhf), -0x1.205966p+3, -inf, 100)
#endif
