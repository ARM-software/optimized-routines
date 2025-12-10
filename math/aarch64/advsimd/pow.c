/*
 * Double-precision vector x^y function.
 *
 * Copyright (c) 2020-2025, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "v_math.h"
#include "test_sig.h"
#include "test_defs.h"

#include "v_pow_inline.h"

static double NOINLINE
pow_scalar_special_case (double x, double y)
{
  uint32_t sign_bias = 0;
  uint64_t ix, iy;
  uint32_t topx, topy;

  ix = asuint64 (x);
  iy = asuint64 (y);
  topx = top12 (x);
  topy = top12 (y);
  /* Special cases: (x < 0x1p-126 or inf or nan) or
     (|y| < 0x1p-65 or |y| >= 0x1p63 or nan).  */
  if (unlikely (topx - SmallPowX >= ThresPowX
		|| (topy & 0x7ff) - SmallPowY >= ThresPowY))
    {
      /* |y| is 0, Inf or NaN.  */
      if (unlikely (zeroinfnan (iy)))
	{
	  if (2 * iy == 0)
	    return issignaling_inline (x) ? x + y : 1.0;
	  if (ix == asuint64 (1.0))
	    return issignaling_inline (y) ? x + y : 1.0;
	  if (2 * ix > 2 * asuint64 (INFINITY)
	      || 2 * iy > 2 * asuint64 (INFINITY))
	    return x + y;
	  if (2 * ix == 2 * asuint64 (1.0))
	    return 1.0;
	  if ((2 * ix < 2 * asuint64 (1.0)) == !(iy >> 63))
	    return 0.0; /* |x|<1 && y==inf or |x|>1 && y==-inf.  */
	  return y * y;
	}
      /* |x| is 0, Inf or NaN.  */
      if (unlikely (zeroinfnan (ix)))
	{
	  double x2 = x * x;
	  if (ix >> 63 && checkint (iy) == 1)
	    {
	      x2 = -x2;
	      sign_bias = 1;
	    }
	  return iy >> 63 ? 1 / x2 : x2;
	}
      /* Here x and y are non-zero finite.  */
      /* Finite negative x returns NaN unless y is integer.  */
      if (ix >> 63)
	{
	  /* Finite x < 0.  */
	  int yint = checkint (iy);
	  if (yint == 0)
	    return __builtin_nan ("");
	  if (yint == 1)
	    sign_bias = SignBias;
	  ix &= 0x7fffffffffffffff;
	  topx &= 0x7ff;
	}
      /* Note: if |y| > 1075 * ln2 * 2^53 ~= 0x1.749p62 then pow(x,y) = inf/0
	 and if |y| < 2^-54 / 1075 ~= 0x1.e7b6p-65 then pow(x,y) = +-1.  */
      if ((topy & 0x7ff) - SmallPowY >= ThresPowY)
	{
	  /* Note: sign_bias == 0 here because y is not odd.  */
	  if (ix == asuint64 (1.0))
	    return 1.0;
	  /* |y| < 2^-65, x^y ~= 1 + y*log(x).  */
	  if ((topy & 0x7ff) < SmallPowY)
	    return 1.0;
	  return (ix > asuint64 (1.0)) == (topy < 0x800) ? INFINITY : 0;
	}
      if (topx == 0)
	{
	  /* Normalize subnormal x so exponent becomes negative.  */
	  ix = asuint64 (x * 0x1p52);
	  ix &= 0x7fffffffffffffff;
	  ix -= 52ULL << 52;
	}
    }

  /* Core computation of exp (y * log (x)).  */
  double lo;
  double hi = log_inline (ix, &lo);
  double ehi = y * hi;
  double elo = y * lo + fma (y, hi, -ehi);
  return exp_inline (ehi, elo, sign_bias);
}

static float64x2_t VPCS_ATTR NOINLINE
scalar_fallback (float64x2_t x, float64x2_t y)
{
  return (float64x2_t){ pow_scalar_special_case (x[0], y[0]),
			pow_scalar_special_case (x[1], y[1]) };
}

/* Implementation of AdvSIMD pow.
   Maximum measured error is 1.04 ULPs:
   _ZGVnN2vv_pow(0x1.024a3e56b3c3p-136, 0x1.87910248b58acp-13)
     got 0x1.f71162f473251p-1
    want 0x1.f71162f473252p-1.  */
float64x2_t VPCS_ATTR V_NAME_D2 (pow) (float64x2_t x, float64x2_t y)
{
  const struct data *d = ptr_barrier (&data);

  /* Case of x <= 0 is too complicated to be vectorised efficiently here,
     fallback to scalar pow for all lanes if any x < 0 detected.  */
  if (v_any_u64 (vclezq_s64 (vreinterpretq_s64_f64 (x))))
    return scalar_fallback (x, y);

  uint64x2_t vix = vreinterpretq_u64_f64 (x);
  uint64x2_t viy = vreinterpretq_u64_f64 (y);

  /* Special cases of x or y.
     The case y==0 does not trigger a special case, since in this case it is
     necessary to fix the result only if x is a signalling nan, which already
     triggers a special case. We test y==0 directly in the scalar fallback.  */
  uint64x2_t x_is_inf_or_nan = vcgeq_u64 (vandq_u64 (vix, d->inf), d->inf);
  uint64x2_t y_is_inf_or_nan = vcgeq_u64 (vandq_u64 (viy, d->inf), d->inf);
  uint64x2_t special = vorrq_u64 (x_is_inf_or_nan, y_is_inf_or_nan);

  /* Fallback to scalar on all lanes if any lane is inf or nan.  */
  if (unlikely (v_any_u64 (special)))
    return scalar_fallback (x, y);

  /* Cases of subnormal x: |x| < 0x1p-1022.  */
  uint64x2_t x_is_subnormal = vcaltq_f64 (x, d->subnormal_bound);
  if (unlikely (v_any_u64 (x_is_subnormal)))
    {
      /* Normalize subnormal x so exponent becomes negative.  */
      uint64x2_t vix_norm = vreinterpretq_u64_f64 (
	  vabsq_f64 (vmulq_f64 (x, d->subnormal_scale)));
      vix_norm = vsubq_u64 (vix_norm, d->subnormal_bias);
      x = vbslq_f64 (x_is_subnormal, vreinterpretq_f64_u64 (vix_norm), x);
    }

  /* Core computation of exp (y * log (x)).  */
  return v_pow_inline (x, y, d);
}

TEST_SIG (V, D, 2, pow)
TEST_ULP (V_NAME_D2 (pow), 0.55)
#define V_POW_INTERVAL2(xlo, xhi, ylo, yhi, n)                                \
  TEST_INTERVAL2 (V_NAME_D2 (pow), xlo, xhi, ylo, yhi, n)                     \
  TEST_INTERVAL2 (V_NAME_D2 (pow), xlo, xhi, -ylo, -yhi, n)                   \
  TEST_INTERVAL2 (V_NAME_D2 (pow), -xlo, -xhi, ylo, yhi, n)                   \
  TEST_INTERVAL2 (V_NAME_D2 (pow), -xlo, -xhi, -ylo, -yhi, n)
#define EXPAND(str) str##000000000
#define SHL52(str) EXPAND (str)
/* Wide intervals spanning the whole domain.  */
V_POW_INTERVAL2 (0, SHL52 (SmallPowX), 0, inf, 40000)
V_POW_INTERVAL2 (SHL52 (SmallPowX), SHL52 (BigPowX), 0, inf, 40000)
V_POW_INTERVAL2 (SHL52 (BigPowX), inf, 0, inf, 40000)
V_POW_INTERVAL2 (0, inf, 0, SHL52 (SmallPowY), 40000)
V_POW_INTERVAL2 (0, inf, SHL52 (SmallPowY), SHL52 (BigPowY), 40000)
V_POW_INTERVAL2 (0, inf, SHL52 (BigPowY), inf, 40000)
V_POW_INTERVAL2 (0, inf, 0, inf, 1000)
/* x~1 or y~1.  */
V_POW_INTERVAL2 (0x1p-1, 0x1p1, 0x1p-10, 0x1p10, 10000)
V_POW_INTERVAL2 (0x1p-500, 0x1p500, 0x1p-1, 0x1p1, 10000)
V_POW_INTERVAL2 (0x1.ep-1, 0x1.1p0, 0x1p8, 0x1p16, 10000)
/* around argmaxs of ULP error.  */
V_POW_INTERVAL2 (0x1p-300, 0x1p-200, 0x1p-20, 0x1p-10, 10000)
V_POW_INTERVAL2 (0x1p50, 0x1p100, 0x1p-20, 0x1p-10, 10000)
/* Common intervals with constrained signs.  */
TEST_INTERVAL2 (V_NAME_D2 (pow), -0.0, -10.0, 0.0, 10.0, 10000)
TEST_INTERVAL2 (V_NAME_D2 (pow), 0.0, 10.0, -0.0, -10.0, 10000)
#define V_POW_SPECIALX(ylo, yhi, n)                                           \
  V_POW_INTERVAL2 (0, 0, ylo, yhi, n)                                         \
  V_POW_INTERVAL2 (1, 1, ylo, yhi, n)                                         \
  V_POW_INTERVAL2 (inf, inf, ylo, yhi, n)                                     \
  V_POW_INTERVAL2 (nan, nan, ylo, yhi, n)                                     \
  V_POW_INTERVAL2 (0xffff000000000000, 0xffff000000000000, ylo, yhi, n)
#define V_POW_SPECIALY(xlo, xhi, n)                                           \
  V_POW_INTERVAL2 (xlo, xhi, 0, 0, n)                                         \
  V_POW_INTERVAL2 (xlo, xhi, inf, inf, n)                                     \
  V_POW_INTERVAL2 (xlo, xhi, nan, nan, n)                                     \
  V_POW_INTERVAL2 (xlo, xhi, 0xffff000000000000, 0xffff000000000000, n)
/* x is 0, inf or nan. |y| is finite.  */
V_POW_SPECIALX (0.0, inf, 1000)
/* x is 0, inf or nan. |y| is special.  */
V_POW_SPECIALX (0.0, 0.0, 1)
V_POW_SPECIALX (inf, inf, 1)
V_POW_SPECIALX (nan, nan, 1)
V_POW_SPECIALX (0xffff000000000000, 0xffff000000000000, 1)
/* |y| is 0, inf or nan. x is finite.  */
V_POW_SPECIALY (0.0, inf, 1000)
/* |y| is 0, inf or nan. x is special.  */
V_POW_SPECIALY (0.0, 0.0, 1)
V_POW_SPECIALY (1.0, 1.0, 1)
V_POW_SPECIALY (inf, inf, 1)
V_POW_SPECIALY (0xffff000000000000, 0xffff000000000000, 1)
/* x is negative.  */
TEST_INTERVAL2 (V_NAME_D2 (pow), -0.0, -inf, 0, 0xffff000000000000, 1000)

/* pow-specific cases, not shared with powr.  */

/* x is negative, y is odd or even integer.  */
TEST_INTERVAL2 (V_NAME_D2 (pow), -0.0, -10.0, 3.0, 3.0, 10000)
TEST_INTERVAL2 (V_NAME_D2 (pow), -0.0, -10.0, 4.0, 4.0, 10000)
/* 1.0^y.  */
TEST_INTERVAL2 (V_NAME_D2 (pow), 1.0, 1.0, 0.0, 0x1p-50, 1000)
TEST_INTERVAL2 (V_NAME_D2 (pow), 1.0, 1.0, 0x1p-50, 1.0, 1000)
TEST_INTERVAL2 (V_NAME_D2 (pow), 1.0, 1.0, 1.0, 0x1p100, 1000)
TEST_INTERVAL2 (V_NAME_D2 (pow), 1.0, 1.0, -1.0, -0x1p120, 1000)
