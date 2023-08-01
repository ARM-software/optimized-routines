/*
 * Single-precision vector exp(x) - 1 function.
 *
 * Copyright (c) 2022-2023, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "v_math.h"
#include "poly_advsimd_f32.h"
#include "pl_sig.h"
#include "pl_test.h"

static const struct data
{
  float32x4_t poly[5];
  float32x4_t invln2, ln2_lo, ln2_hi, shift;
} data = {
  /* Generated using fpminimax with degree=5 in [-log(2)/2, log(2)/2].  */
  .poly = { V4 (0x1.fffffep-2), V4 (0x1.5554aep-3), V4 (0x1.555736p-5),
	    V4 (0x1.12287cp-7), V4 (0x1.6b55a2p-10) },
  .invln2 = V4 (0x1.715476p+0f),
  .ln2_hi = V4 (0x1.62e4p-1f),
  .ln2_lo = V4 (0x1.7f7d1cp-20f),
  .shift = V4 (0x1.8p23f),
};

#define AllMask v_u32 (0xffffffff)
#define AbsMask v_u32 (0x7fffffff)
#define SignMask v_u32 (0x80000000)
/* Largest value of x for which expm1(x) should round to -1.  */
#define BigBound 0x42af5e20		/* asuint(0x1.5ebc4p+6).  */
#define BigBoundNeg 0x42cddd5e		/* asuint(0x1.9bbabcp+6).  */
#define TinyBound 0x34000000		/* asuint(0x1p-23).  */
#define ExponentBias v_s32 (0x3f800000) /* asuint(1.0f).  */

static float32x4_t VPCS_ATTR NOINLINE
special_case (float32x4_t x, float32x4_t y, uint32x4_t special)
{
  return v_call_f32 (expm1f, x, y, special);
}

/* Single-precision vector exp(x) - 1 function.
   The maximum error is 1.51 ULP:
   _ZGVnN4v_expm1f (0x1.8baa96p-2) got 0x1.e2fb9p-2
				  want 0x1.e2fb94p-2.  */
float32x4_t VPCS_ATTR V_NAME_F1 (expm1) (float32x4_t x)
{
  const struct data *d = ptr_barrier (&data);

  uint32x4_t ix = vreinterpretq_u32_f32 (x);
  uint32x4_t ax = vandq_u32 (ix, AbsMask);

#if WANT_SIMD_EXCEPT
  /* If fp exceptions are to be triggered correctly, fall back to the scalar
     variant for all lanes if any of them should trigger an exception.  */
  uint32x4_t special = vorrq_u32 (vcgeq_u32 (ax, v_u32 (BigBound)),
				  vcleq_u32 (ax, v_u32 (TinyBound)));
  special = vorrq_u32 (special, vceqq_u32 (ix, SignMask));
  if (unlikely (v_any_u32 (special)))
    return special_case (x, x, AllMask);
#else
  /* Handles very large values (+ve and -ve), +/-NaN, +/-Inf and -0.  */
  uint32x4_t special
      = vorrq_u32 (vcgeq_u32 (ax, v_u32 (BigBound)), vceqq_u32 (ix, SignMask));
#endif

  /* Reduce argument to smaller range:
     Let i = round(x / ln2)
     and f = x - i * ln2, then f is in [-ln2/2, ln2/2].
     exp(x) - 1 = 2^i * (expm1(f) + 1) - 1
     where 2^i is exact because i is an integer.  */
  float32x4_t j = vsubq_f32 (vfmaq_f32 (d->shift, d->invln2, x), d->shift);
  int32x4_t i = vcvtq_s32_f32 (j);
  float32x4_t f = vfmsq_f32 (x, j, d->ln2_hi);
  f = vfmsq_f32 (f, j, d->ln2_lo);

  /* Approximate expm1(f) using polynomial.
     Taylor expansion for expm1(x) has the form:
	 x + ax^2 + bx^3 + cx^4 ....
     So we calculate the polynomial P(f) = a + bf + cf^2 + ...
     and assemble the approximation expm1(f) ~= f + f^2 * P(f).  */
  float32x4_t p = v_horner_4_f32 (f, d->poly);
  p = vfmaq_f32 (f, vmulq_f32 (f, f), p);

  /* Assemble the result.
     expm1(x) ~= 2^i * (p + 1) - 1
     Let t = 2^i.  */
  int32x4_t u = vaddq_s32 (vshlq_n_s32 (i, 23), ExponentBias);
  float32x4_t t = vreinterpretq_f32_s32 (u);
  /* expm1(x) ~= p * t + (t - 1).  */
  float32x4_t y = vfmaq_f32 (vsubq_f32 (t, v_f32 (1.0f)), p, t);

#if !WANT_SIMD_EXCEPT
  if (unlikely (v_any_u32 (special)))
    return special_case (x, y, special);
#endif

  return y;
}

PL_SIG (V, F, 1, expm1, -9.9, 9.9)
PL_TEST_ULP (V_NAME_F1 (expm1), 1.02)
PL_TEST_EXPECT_FENV (V_NAME_F1 (expm1), WANT_SIMD_EXCEPT)
PL_TEST_INTERVAL (V_NAME_F1 (expm1), 0, TinyBound, 1000)
PL_TEST_INTERVAL (V_NAME_F1 (expm1), -0, -TinyBound, 1000)
PL_TEST_INTERVAL (V_NAME_F1 (expm1), TinyBound, BigBound, 1000000)
PL_TEST_INTERVAL (V_NAME_F1 (expm1), -TinyBound, -BigBoundNeg, 1000000)
