/*
 * Double-precision vector sinh(x) function.
 *
 * Copyright (c) 2022-2023, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "v_math.h"
#include "poly_advsimd_f64.h"
#include "pl_sig.h"
#include "pl_test.h"

#define AbsMask 0x7fffffffffffffff
#define Half 0x3fe0000000000000
#define BigBound                                                               \
  0x4080000000000000 /* 2^9. expm1 helper overflows for large input.  */
#define TinyBound                                                              \
  0x3e50000000000000 /* 2^-26, below which sinh(x) rounds to x.  */
#define InvLn2 v_f64 (0x1.71547652b82fep0)
#define MLn2hi v_f64 (-0x1.62e42fefa39efp-1)
#define MLn2lo v_f64 (-0x1.abc9e3b39803fp-56)
#define Shift v_f64 (0x1.8p52)
#define One 0x3ff0000000000000

/* Generated using Remez, deg=12 in [-log(2)/2, log(2)/2].  */
const static volatile float64x2_t expm1_poly[11]
    = { V2 (0x1p-1),
	V2 (0x1.5555555555559p-3),
	V2 (0x1.555555555554bp-5),
	V2 (0x1.111111110f663p-7),
	V2 (0x1.6c16c16c1b5f3p-10),
	V2 (0x1.a01a01affa35dp-13),
	V2 (0x1.a01a018b4ecbbp-16),
	V2 (0x1.71ddf82db5bb4p-19),
	V2 (0x1.27e517fc0d54bp-22),
	V2 (0x1.af5eedae67435p-26),
	V2 (0x1.1f143d060a28ap-29) };

static inline float64x2_t
expm1_inline (float64x2_t x)
{
  /* Reduce argument:
     exp(x) - 1 = 2^i * (expm1(f) + 1) - 1
     where i = round(x / ln2)
     and   f = x - i * ln2 (f in [-ln2/2, ln2/2]).  */
  float64x2_t j = vfmaq_f64 (Shift, InvLn2, x) - Shift;
  int64x2_t i = vcvtq_s64_f64 (j);
  float64x2_t f = vfmaq_f64 (x, j, MLn2hi);
  f = vfmaq_f64 (f, j, MLn2lo);
  /* Approximate expm1(f) using polynomial.  */
  float64x2_t f2 = f * f, f4 = f2 * f2, f8 = f4 * f4;
  float64x2_t p = vfmaq_f64 (
      f, f2,
      v_estrin_10_f64 (f, f2, f4, f8, (const float64x2_t *) expm1_poly));
  /* t = 2^i.  */
  float64x2_t t = vreinterpretq_f64_u64 (vreinterpretq_u64_s64 (i << 52) + One);
  /* expm1(x) ~= p * t + (t - 1).  */
  return vfmaq_f64 (t - 1, p, t);
}

static NOINLINE VPCS_ATTR float64x2_t
special_case (float64x2_t x)
{
  return v_call_f64 (sinh, x, x, v_u64 (-1));
}

/* Approximation for vector double-precision sinh(x) using expm1.
   sinh(x) = (exp(x) - exp(-x)) / 2.
   The greatest observed error is 2.57 ULP:
   sinh(0x1.9fb1d49d1d58bp-2) got 0x1.ab34e59d678dcp-2
			     want 0x1.ab34e59d678d9p-2.  */
VPCS_ATTR float64x2_t V_NAME_D1 (sinh) (float64x2_t x)
{
  uint64x2_t ix = vreinterpretq_u64_f64 (x);
  uint64x2_t iax = ix & AbsMask;
  float64x2_t ax = vreinterpretq_f64_u64 (iax);
  uint64x2_t sign = ix & ~AbsMask;
  float64x2_t halfsign = vreinterpretq_f64_u64 (sign | Half);

#if WANT_SIMD_EXCEPT
  uint64x2_t special = (iax - TinyBound) >= (BigBound - TinyBound);
#else
  uint64x2_t special = iax >= BigBound;
#endif

  /* Fall back to scalar variant for all lanes if any of them are special.  */
  if (unlikely (v_any_u64 (special)))
    return special_case (x);

  /* Up to the point that expm1 overflows, we can use it to calculate sinh
     using a slight rearrangement of the definition of sinh. This allows us to
     retain acceptable accuracy for very small inputs.  */
  float64x2_t t = expm1_inline (ax);
  return (t + t / (t + 1)) * halfsign;
}

PL_SIG (V, D, 1, sinh, -10.0, 10.0)
PL_TEST_ULP (V_NAME_D1 (sinh), 2.08)
PL_TEST_EXPECT_FENV (V_NAME_D1 (sinh), WANT_SIMD_EXCEPT)
PL_TEST_INTERVAL (V_NAME_D1 (sinh), 0, TinyBound, 1000)
PL_TEST_INTERVAL (V_NAME_D1 (sinh), -0, -TinyBound, 1000)
PL_TEST_INTERVAL (V_NAME_D1 (sinh), TinyBound, BigBound, 500000)
PL_TEST_INTERVAL (V_NAME_D1 (sinh), -TinyBound, -BigBound, 500000)
PL_TEST_INTERVAL (V_NAME_D1 (sinh), BigBound, inf, 1000)
PL_TEST_INTERVAL (V_NAME_D1 (sinh), -BigBound, -inf, 1000)
