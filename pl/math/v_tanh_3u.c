/*
 * Double-precision vector tanh(x) function.
 * Copyright (c) 2023, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "v_math.h"
#include "poly_advsimd_f64.h"
#include "mathlib.h"
#include "pl_sig.h"
#include "pl_test.h"

#define AbsMask v_u64 (0x7fffffffffffffff)
#define InvLn2 v_f64 (0x1.71547652b82fep0)
#define MLn2hi v_f64 (-0x1.62e42fefa39efp-1)
#define MLn2lo v_f64 (-0x1.abc9e3b39803fp-56)
#define Shift v_f64 (0x1.8p52)

#define BoringBound 0x403241bf835f9d5f /* asuint64 (0x1.241bf835f9d5fp+4).  */
#define TinyBound 0x3e40000000000000   /* asuint64 (0x1p-27).  */
#define One v_u64 (0x3ff0000000000000)

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
  /* Helper routine for calculating exp(x) - 1. Vector port of the helper from
     the scalar variant of tanh.  */

  /* Reduce argument: f in [-ln2/2, ln2/2], i is exact.  */
  float64x2_t j = vfmaq_f64 (Shift, InvLn2, x) - Shift;
  int64x2_t i = vcvtq_s64_f64 (j);
  float64x2_t f = vfmaq_f64 (x, j, MLn2hi);
  f = vfmaq_f64 (f, j, MLn2lo);

  /* Approximate expm1(f) using polynomial.  */
  float64x2_t f2 = f * f;
  float64x2_t f4 = f2 * f2;
  float64x2_t p = vfmaq_f64 (
      f, f2,
      v_estrin_10_f64 (f, f2, f4, f4 * f4, (const float64x2_t *) expm1_poly));

  /* t = 2 ^ i.  */
  float64x2_t t = vreinterpretq_f64_u64 (vreinterpretq_u64_s64 (i << 52) + One);
  /* expm1(x) = p * t + (t - 1).  */
  return vfmaq_f64 (t - 1, p, t);
}

static NOINLINE float64x2_t
special_case (float64x2_t x, float64x2_t y, uint64x2_t special)
{
  return v_call_f64 (tanh, x, y, special);
}

/* Vector approximation for double-precision tanh(x), using a simplified
   version of expm1. The greatest observed error is 2.75 ULP:
   __v_tanh(-0x1.c143c3a44e087p-3) got -0x1.ba31ba4691ab7p-3
				  want -0x1.ba31ba4691ab4p-3.  */
VPCS_ATTR float64x2_t V_NAME_D1 (tanh) (float64x2_t x)
{
  uint64x2_t ix = vreinterpretq_u64_f64 (x);
  uint64x2_t ia = ix & AbsMask;

  /* Trigger special-cases for tiny, boring and infinity/NaN.  */
  uint64x2_t special = (ia - TinyBound) > (BoringBound - TinyBound);
  float64x2_t u;

  /* To trigger fp exceptions correctly, set special lanes to a neutral value.
     They will be fixed up later by the special-case handler.  */
  if (unlikely (v_any_u64 (special)))
    u = vbslq_f64 (special, v_f64 (1), x) * 2;
  else
    u = x * 2;

  /* tanh(x) = (e^2x - 1) / (e^2x + 1).  */
  float64x2_t q = expm1_inline (u);
  float64x2_t y = q / (q + 2);

  if (unlikely (v_any_u64 (special)))
    return special_case (x, y, special);
  return y;
}

PL_SIG (V, D, 1, tanh, -10.0, 10.0)
PL_TEST_ULP (V_NAME_D1 (tanh), 2.26)
PL_TEST_EXPECT_FENV_ALWAYS (V_NAME_D1 (tanh))
PL_TEST_INTERVAL (V_NAME_D1 (tanh), 0, TinyBound, 1000)
PL_TEST_INTERVAL (V_NAME_D1 (tanh), -0, -TinyBound, 1000)
PL_TEST_INTERVAL (V_NAME_D1 (tanh), TinyBound, BoringBound, 100000)
PL_TEST_INTERVAL (V_NAME_D1 (tanh), -TinyBound, -BoringBound, 100000)
PL_TEST_INTERVAL (V_NAME_D1 (tanh), BoringBound, inf, 1000)
PL_TEST_INTERVAL (V_NAME_D1 (tanh), -BoringBound, -inf, 1000)
