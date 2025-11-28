/*
 * Double-precision SVE exp(y * log(x)) function.
 *
 * Copyright (c) 2025, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "sv_math.h"
#include "test_sig.h"
#include "test_defs.h"
#define WANT_SV_POW_SIGN_BIAS 0
#include "sv_pow_inline.h"

/* A scalar subroutine used to fix main powr special cases. Similar to the
   preamble of scalar pow except that we do not update ix and sign_bias.  */
static inline double
powr_specialcase (double x, double y)
{
  uint64_t ix = asuint64 (x);
  uint64_t iy = asuint64 (y);
  if (unlikely (zeroinfnan (iy)))
    {
      /* |y| is 0.0.  */
      if (2 * iy == 0)
	return issignaling_inline (x) ? x + y : 1.0;
      /* x is 1.0.  */
      if (ix == asuint64 (1.0))
	return issignaling_inline (y) ? x + y : 1.0;
      /* |x| or |y| is nan.  */
      if (2 * ix > 2 * asuint64 (INFINITY) || 2 * iy > 2 * asuint64 (INFINITY))
	return x + y;
      /* |x|<1 && y==inf or |x|>1 && y==-inf.  */
      if ((2 * ix < 2 * asuint64 (1.0)) == !(iy >> 63))
	return 0.0;
      /* Remaining cases should be y is inf and x is finite, but do not satisfy
	 above case for returning 0.0 therefore returns +inf.  */
      return y * y;
    }
  if (unlikely (zeroinfnan (ix)))
    {
      double x2 = x * x;
      return (iy >> 63) ? 1 / x2 : x2;
    }
  /* Return x for convenience, but make sure result is never used.  */
  return x;
}

/* Scalar fallback for special case routines with custom signature.  */
static svfloat64_t NOINLINE
sv_powr_specialcase (svfloat64_t x1, svfloat64_t x2, svfloat64_t y,
		     svbool_t cmp)
{
  return sv_call2_f64 (powr_specialcase, x1, x2, y, cmp);
}

/* Implementation of SVE powr.

   Provides the same accuracy as AdvSIMD pow and powr, since it relies on the
   same algorithm.

   Maximum measured error is 1.04 ULPs:
   SV_NAME_D2 (powr) (0x1.3d2d45bc848acp+63, -0x1.a48a38b40cd43p-12)
     got 0x1.f7116284221fcp-1
    want 0x1.f7116284221fdp-1.  */
svfloat64_t SV_NAME_D2 (powr) (svfloat64_t x, svfloat64_t y, const svbool_t pg)
{
  const struct data *d = ptr_barrier (&data);

  svuint64_t vix = svreinterpret_u64 (x);
  svuint64_t viy = svreinterpret_u64 (y);

  svbool_t xpos = svcmpgt (pg, x, sv_f64 (0.0));

  /* Special cases of x or y: zero, inf and nan.  */
  svbool_t xspecial = sv_zeroinfnan (pg, vix);
  svbool_t yspecial = sv_zeroinfnan (pg, viy);
  svbool_t cmp = svorr_z (pg, xspecial, yspecial);

  /* Cases of positive subnormal x: 0 < x < 0x1p-1022.  */
  svbool_t x_is_subnormal = svaclt (xpos, x, 0x1p-1022);
  if (unlikely (svptest_any (xpos, x_is_subnormal)))
    {
      /* Normalize subnormal x so exponent becomes negative.  */
      svuint64_t vix_norm
	  = svreinterpret_u64 (svmul_m (x_is_subnormal, x, 0x1p52));
      vix = svsub_m (x_is_subnormal, vix_norm, 52ULL << 52);
    }

  svfloat64_t vlo;
  svfloat64_t vhi = sv_log_inline (xpos, vix, &vlo, d);

  svfloat64_t vehi = svmul_x (svptrue_b64 (), y, vhi);
  svfloat64_t vemi = svmls_x (xpos, vehi, y, vhi);
  svfloat64_t velo = svnmls_x (xpos, vemi, y, vlo);
  svfloat64_t vz = sv_exp_inline (xpos, vehi, velo, sv_u64 (0), d);

  /* Cases of negative x.  */
  vz = svsel (xpos, vz, sv_f64 (__builtin_nan ("")));

  if (unlikely (svptest_any (cmp, cmp)))
    return sv_powr_specialcase (x, y, vz, cmp);

  return vz;
}

#if WANT_C23_TESTS
TEST_ULP (SV_NAME_D2 (powr), 0.55)
/* Wide intervals spanning the positive domain.  */
#  define SV_POWR_INTERVAL2(xlo, xhi, ylo, yhi, n)                            \
    TEST_INTERVAL2 (SV_NAME_D2 (powr), xlo, xhi, ylo, yhi, n)                 \
    TEST_INTERVAL2 (SV_NAME_D2 (powr), xlo, xhi, -ylo, -yhi, n)
SV_POWR_INTERVAL2 (0, 0x1p-1022, 0, inf, 40000)
SV_POWR_INTERVAL2 (0x1p-1022, 1, 0, inf, 50000)
SV_POWR_INTERVAL2 (1, inf, 0, inf, 50000)
/* x~1 or y~1.  */
SV_POWR_INTERVAL2 (0x1p-1, 0x1p1, 0x1p-10, 0x1p10, 10000)
SV_POWR_INTERVAL2 (0x1.ep-1, 0x1.1p0, 0x1p8, 0x1p16, 10000)
SV_POWR_INTERVAL2 (0x1p-500, 0x1p500, 0x1p-1, 0x1p1, 10000)
/* around estimated argmaxs of ULP error.  */
SV_POWR_INTERVAL2 (0x1p-300, 0x1p-200, 0x1p-20, 0x1p-10, 10000)
SV_POWR_INTERVAL2 (0x1p50, 0x1p100, 0x1p-20, 0x1p-10, 10000)
/* |x| is 0, inf or nan.  */
SV_POWR_INTERVAL2 (0.0, 0.0, 0, inf, 1000)
SV_POWR_INTERVAL2 (inf, inf, 0, inf, 1000)
SV_POWR_INTERVAL2 (nan, nan, 0, inf, 1000)
/* |y| is 0, inf or nan.  */
SV_POWR_INTERVAL2 (0, inf, 0.0, 0.0, 1000)
SV_POWR_INTERVAL2 (0, inf, inf, inf, 1000)
SV_POWR_INTERVAL2 (0, inf, nan, nan, 1000)
/* x is negative.  */
TEST_INTERVAL2 (SV_NAME_D2 (powr), -0.0, -inf, 0, 0xffff000000000000, 1000)
#endif
CLOSE_SVE_ATTR
