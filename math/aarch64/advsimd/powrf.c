/*
 * Single-precision vector exp(y * log(x)) function.
 *
 * Copyright (c) 2025, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "v_math.h"
#include "v_powrf_inline.h"
#include "test_defs.h"

/* A scalar subroutine used to fix main powrf special cases.  */
static inline float
powrf_specialcase (float x, float y)
{
  /* Negative x returns NaN (+0/-0 and NaN x not handled here).  */
  if (x < 0)
    return __builtin_nanf ("");

  uint32_t ix = asuint (x);
  uint32_t iy = asuint (y);
  /* y is 0, Inf or NaN.  */
  if (unlikely (zeroinfnan (iy)))
    {
      /* |x| or |y| is NaN.  */
      if (2 * ix > 2u * 0x7f800000 || 2 * iy > 2u * 0x7f800000)
	return __builtin_nanf ("");
      /* |y| = 0.  */
      if (2 * iy == 0)
	{
	  /* |x| = 0 or inf.  */
	  if ((2 * ix == 0) || (2 * ix == 2u * 0x7f800000))
	    return __builtin_nanf ("");
	  /* x is finite.  */
	  return 1.0f;
	}
      /* |y| = Inf and x = 1.0.  */
      if (ix == 0x3f800000)
	return __builtin_nanf ("");
      /* |x| < 1 and y = Inf or |x| > 1 and y = -Inf.  */
      if ((2 * ix < 2 * 0x3f800000) == !(iy & 0x80000000))
	return 0.0f;
      /* |y| = Inf and previous conditions not met.  */
      return y * y;
    }
  /* x is 0, Inf or NaN. Negative x are handled in the core.  */
  if (unlikely (zeroinfnan (ix)))
    {
      float x2 = x * x;
      return iy & 0x80000000 ? 1 / x2 : x2;
    }

  /* Return x for convenience, but make sure result is never used.  */
  return x;
}

/* Special case function wrapper.  */
static float32x4_t VPCS_ATTR NOINLINE
special_case (float32x4_t x, float32x4_t y, float32x4_t ret, uint32x4_t cmp)
{
  return v_call2_f32 (powrf_specialcase, x, y, ret, cmp);
}

/* Power implementation for x containing negative or subnormal lanes.  */
static inline float32x4_t
v_powrf_x_is_neg_or_sub (float32x4_t x, float32x4_t y, const struct data *d)
{
  uint32x4_t xsmall = vcaltq_f32 (x, v_f32 (0x1p-126f));

  /* Normalize subnormals.  */
  float32x4_t a = vabsq_f32 (x);
  uint32x4_t ia_norm = vreinterpretq_u32_f32 (vmulq_f32 (a, d->norm));
  ia_norm = vsubq_u32 (ia_norm, d->subnormal_bias);
  a = vbslq_f32 (xsmall, vreinterpretq_f32_u32 (ia_norm), a);

  /* Evaluate exp (y * log(x)) using |x| and sign bias correction.  */
  float32x4_t ret = v_powrf_core (a, y, d);

  /* Cases of finite y and finite negative x.  */
  uint32x4_t xisneg = vcltzq_f32 (x);
  return vbslq_f32 (xisneg, d->nan, ret);
}

/* Implementation of AdvSIMD powrf.

     powr(x,y) := exp(y * log (x))

   This means powr(x,y) core computation matches that of pow(x,y)
   but powr returns NaN for negative x even if y is an integer.

   Maximum measured error is 2.57 ULPs:
   V_NAME_F2 (powr) (0x1.031706p+0, 0x1.ce2ec2p+12)
     got 0x1.fff868p+127
    want 0x1.fff862p+127.  */
float32x4_t VPCS_ATTR NOINLINE V_NAME_F2 (powr) (float32x4_t x, float32x4_t y)
{
  const struct data *d = ptr_barrier (&data);

  /* Special cases of x or y: zero, inf and nan.  */
  uint32x4_t ix = vreinterpretq_u32_f32 (x);
  uint32x4_t iy = vreinterpretq_u32_f32 (y);
  uint32x4_t xspecial = v_zeroinfnan (d, ix);
  uint32x4_t yspecial = v_zeroinfnan (d, iy);
  uint32x4_t cmp = vorrq_u32 (xspecial, yspecial);

  /* Evaluate pow(x, y) for x containing negative or subnormal lanes.  */
  uint32x4_t x_is_neg_or_sub = vcltq_f32 (x, v_f32 (0x1p-126f));
  if (unlikely (v_any_u32 (x_is_neg_or_sub)))
    {
      float32x4_t ret = v_powrf_x_is_neg_or_sub (x, y, d);
      if (unlikely (v_any_u32 (cmp)))
	return special_case (x, y, ret, cmp);
      return ret;
    }

  /* Else evaluate pow(x, y) for normal and positive x only.  */
  if (unlikely (v_any_u32 (cmp)))
    return special_case (x, y, v_powrf_core (x, y, d), cmp);
  return v_powrf_core (x, y, d);
}

HALF_WIDTH_ALIAS_F2 (powr)

#if WANT_C23_TESTS
TEST_ULP (V_NAME_F2 (powr), 2.08)
#  define V_POWRF_INTERVAL2(xlo, xhi, ylo, yhi, n)                            \
    TEST_INTERVAL2 (V_NAME_F2 (powr), xlo, xhi, ylo, yhi, n)                  \
    TEST_INTERVAL2 (V_NAME_F2 (powr), xlo, xhi, -ylo, -yhi, n)                \
    TEST_INTERVAL2 (V_NAME_F2 (powr), -xlo, -xhi, ylo, yhi, n)                \
    TEST_INTERVAL2 (V_NAME_F2 (powr), -xlo, -xhi, -ylo, -yhi, n)
/* Wide intervals spanning the whole domain.  */
V_POWRF_INTERVAL2 (0, 0x1p-126, 0, inf, 40000)
V_POWRF_INTERVAL2 (0x1p-126, 1, 0, inf, 50000)
V_POWRF_INTERVAL2 (1, inf, 0, inf, 50000)
/* x~1 or y~1.  */
V_POWRF_INTERVAL2 (0x1p-1, 0x1p1, 0x1p-7, 0x1p7, 50000)
V_POWRF_INTERVAL2 (0x1p-70, 0x1p70, 0x1p-1, 0x1p1, 50000)
V_POWRF_INTERVAL2 (0x1.ep-1, 0x1.1p0, 0x1p8, 0x1p14, 50000)
#  define V_POWRF_SPECIALX(ylo, yhi, n)                                       \
    V_POWRF_INTERVAL2 (0, 0, ylo, yhi, n)                                     \
    V_POWRF_INTERVAL2 (1, 1, ylo, yhi, n)                                     \
    V_POWRF_INTERVAL2 (inf, inf, ylo, yhi, n)                                 \
    V_POWRF_INTERVAL2 (nan, nan, ylo, yhi, n)                                 \
    V_POWRF_INTERVAL2 (0xffff0000, 0xffff0000, ylo, yhi, n)
#  define V_POWRF_SPECIALY(xlo, xhi, n)                                       \
    V_POWRF_INTERVAL2 (xlo, xhi, 0, 0, n)                                     \
    V_POWRF_INTERVAL2 (xlo, xhi, inf, inf, n)                                 \
    V_POWRF_INTERVAL2 (xlo, xhi, nan, nan, n)                                 \
    V_POWRF_INTERVAL2 (xlo, xhi, 0xffff0000, 0xffff0000, n)
/* x is 0, inf or nan. |y| is finite.  */
V_POWRF_SPECIALX (0.0, inf, 1000)
/* x is 0, inf or nan. |y| is special.  */
V_POWRF_SPECIALX (0.0, 0.0, 1)
V_POWRF_SPECIALX (inf, inf, 1)
V_POWRF_SPECIALX (nan, nan, 1)
V_POWRF_SPECIALX (0xffff0000, 0xffff0000, 1)
/* |y| is 0, inf or nan. x is finite.  */
V_POWRF_SPECIALY (0.0, inf, 1000)
/* |y| is 0, inf or nan. x is special.  */
V_POWRF_SPECIALY (0.0, 0.0, 1)
V_POWRF_SPECIALY (1.0, 1.0, 1)
V_POWRF_SPECIALY (inf, inf, 1)
V_POWRF_SPECIALY (0xffff0000, 0xffff0000, 1)
/* x is negative.  */
TEST_INTERVAL2 (V_NAME_F2 (powr), -0.0, -inf, 0, 0xffff0000, 1000)
#endif
