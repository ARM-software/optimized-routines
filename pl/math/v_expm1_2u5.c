/*
 * Double-precision vector exp(x) - 1 function.
 *
 * Copyright (c) 2022-2023, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "v_math.h"
#include "pl_sig.h"
#include "pl_test.h"

#define InvLn2 v_f64 (0x1.71547652b82fep0)
#define MLn2hi v_f64 (-0x1.62e42fefa39efp-1)
#define MLn2lo v_f64 (-0x1.abc9e3b39803fp-56)
#define Shift v_f64 (0x1.8p52)
#define TinyBound                                                              \
  0x3cc0000000000000 /* 0x1p-51, below which expm1(x) is within 2 ULP of x. */
#define SpecialBound                                                           \
  0x40862b7d369a5aa9 /* 0x1.62b7d369a5aa9p+9. For |x| > SpecialBound, the      \
			final stage of the algorithm overflows so fall back to \
			scalar.  */
#define AbsMask 0x7fffffffffffffff
#define One 0x3ff0000000000000

#define C(i) v_f64 (__expm1_poly[i])

static inline float64x2_t
eval_poly (float64x2_t f, float64x2_t f2)
{
  /* Evaluate custom polynomial using Estrin scheme.  */
  float64x2_t p_01 = vfmaq_f64 (C (0), f, C (1));
  float64x2_t p_23 = vfmaq_f64 (C (2), f, C (3));
  float64x2_t p_45 = vfmaq_f64 (C (4), f, C (5));
  float64x2_t p_67 = vfmaq_f64 (C (6), f, C (7));
  float64x2_t p_89 = vfmaq_f64 (C (8), f, C (9));

  float64x2_t p_03 = vfmaq_f64 (p_01, f2, p_23);
  float64x2_t p_47 = vfmaq_f64 (p_45, f2, p_67);
  float64x2_t p_8a = vfmaq_f64 (p_89, f2, C (10));

  float64x2_t f4 = f2 * f2;
  float64x2_t p_07 = vfmaq_f64 (p_03, f4, p_47);
  return vfmaq_f64 (p_07, f4 * f4, p_8a);
}

/* Double-precision vector exp(x) - 1 function.
   The maximum error observed error is 2.18 ULP:
   __v_expm1(0x1.634ba0c237d7bp-2) got 0x1.a8b9ea8d66e22p-2
				  want 0x1.a8b9ea8d66e2p-2.  */
VPCS_ATTR
float64x2_t V_NAME_D1 (expm1) (float64x2_t x)
{
  uint64x2_t ix = v_as_u64_f64 (x);
  uint64x2_t ax = ix & AbsMask;

#if WANT_SIMD_EXCEPT
  /* If fp exceptions are to be triggered correctly, fall back to the scalar
     variant for all lanes if any of them should trigger an exception.  */
  uint64x2_t special = (ax >= SpecialBound) | (ax <= TinyBound);
  if (unlikely (v_any_u64 (special)))
    return v_call_f64 (expm1, x, x, v_u64 (-1));
#else
  /* Large input, NaNs and Infs.  */
  uint64x2_t special = (ax >= SpecialBound) | (ix == 0x8000000000000000);
#endif

  /* Reduce argument to smaller range:
     Let i = round(x / ln2)
     and f = x - i * ln2, then f is in [-ln2/2, ln2/2].
     exp(x) - 1 = 2^i * (expm1(f) + 1) - 1
     where 2^i is exact because i is an integer.  */
  float64x2_t j = vfmaq_f64 (Shift, InvLn2, x) - Shift;
  int64x2_t i = vcvtq_s64_f64 (j);
  float64x2_t f = vfmaq_f64 (x, j, MLn2hi);
  f = vfmaq_f64 (f, j, MLn2lo);

  /* Approximate expm1(f) using polynomial.
     Taylor expansion for expm1(x) has the form:
	 x + ax^2 + bx^3 + cx^4 ....
     So we calculate the polynomial P(f) = a + bf + cf^2 + ...
     and assemble the approximation expm1(f) ~= f + f^2 * P(f).  */
  float64x2_t f2 = f * f;
  float64x2_t p = vfmaq_f64 (f, f2, eval_poly (f, f2));

  /* Assemble the result.
     expm1(x) ~= 2^i * (p + 1) - 1
     Let t = 2^i.  */
  float64x2_t t = v_as_f64_u64 (v_as_u64_s64 (i << 52) + One);
  /* expm1(x) ~= p * t + (t - 1).  */
  float64x2_t y = vfmaq_f64 (t - 1, p, t);

#if !WANT_SIMD_EXCEPT
  if (unlikely (v_any_u64 (special)))
    return v_call_f64 (expm1, x, y, special);
#endif

  return y;
}

PL_SIG (V, D, 1, expm1, -9.9, 9.9)
PL_TEST_ULP (V_NAME_D1 (expm1), 1.68)
PL_TEST_EXPECT_FENV (V_NAME_D1 (expm1), WANT_SIMD_EXCEPT)
PL_TEST_INTERVAL (V_NAME_D1 (expm1), 0, 0x1p-51, 1000)
PL_TEST_INTERVAL (V_NAME_D1 (expm1), -0, -0x1p-51, 1000)
PL_TEST_INTERVAL (V_NAME_D1 (expm1), 0x1p-51, 0x1.63108c75a1937p+9, 100000)
PL_TEST_INTERVAL (V_NAME_D1 (expm1), -0x1p-51, -0x1.740bf7c0d927dp+9, 100000)
PL_TEST_INTERVAL (V_NAME_D1 (expm1), 0x1.63108c75a1937p+9, inf, 100)
PL_TEST_INTERVAL (V_NAME_D1 (expm1), -0x1.740bf7c0d927dp+9, -inf, 100)
