/*
 * Single-precision SVE x^y function.
 *
 * Copyright (c) 2023-2025, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "sv_math.h"
#include "test_sig.h"
#include "test_defs.h"
#define WANT_SV_POWF_SIGN_BIAS 1
#include "sv_powf_inline.h"

/* A scalar subroutine used to fix main power special cases. Similar to the
   preamble of scalar powf except that we do not update ix and sign_bias. This
   is done in the preamble of the SVE powf.  */
static inline float
powf_specialcase (float x, float y)
{
  uint32_t ix = asuint (x);
  uint32_t iy = asuint (y);
  if (unlikely (zeroinfnan (iy)))
    {
      if (2 * iy == 0)
	return issignalingf_inline (x) ? x + y : 1.0f;
      if (ix == 0x3f800000)
	return issignalingf_inline (y) ? x + y : 1.0f;
      if (2 * ix > 2u * 0x7f800000 || 2 * iy > 2u * 0x7f800000)
	return x + y;
      if (2 * ix == 2 * 0x3f800000)
	return 1.0f;
      if ((2 * ix < 2 * 0x3f800000) == !(iy & 0x80000000))
	return 0.0f; /* |x|<1 && y==inf or |x|>1 && y==-inf.  */
      return y * y;
    }
  if (unlikely (zeroinfnan (ix)))
    {
      float x2 = x * x;
      if (ix & 0x80000000 && checkint (iy) == 1)
	x2 = -x2;
      return iy & 0x80000000 ? 1 / x2 : x2;
    }
  /* Return x for convenience, but make sure result is never used.  */
  return x;
}

/* Scalar fallback for special case routines with custom signature.  */
static svfloat32_t NOINLINE
sv_call_powf_sc (svfloat32_t x1, svfloat32_t x2, svfloat32_t y, svbool_t cmp)
{
  return sv_call2_f32 (powf_specialcase, x1, x2, y, cmp);
}

/* Implementation of SVE powf.

   Provides the same accuracy as AdvSIMD powf, since it relies on the same
   algorithm.

   Maximum measured error is 2.57 ULPs:
   SV_NAME_F2 (pow) (0x1.031706p+0, 0x1.ce2ec2p+12)
     got 0x1.fff868p+127
    want 0x1.fff862p+127.  */
svfloat32_t SV_NAME_F2 (pow) (svfloat32_t x, svfloat32_t y, const svbool_t pg)
{
  const struct data *d = ptr_barrier (&data);

  svuint32_t vix0 = svreinterpret_u32 (x);
  svuint32_t viy0 = svreinterpret_u32 (y);

  /* Negative x cases.  */
  svbool_t xisneg = svcmplt (pg, x, sv_f32 (0));

  /* Set sign_bias and ix depending on sign of x and nature of y.  */
  svbool_t yint_or_xpos = pg;
  svuint32_t sign_bias = sv_u32 (0);
  svuint32_t vix = vix0;
  if (unlikely (svptest_any (pg, xisneg)))
    {
      /* Determine nature of y.  */
      yint_or_xpos = svisint (xisneg, y);
      svbool_t yisodd_xisneg = svisodd (xisneg, y);
      /* ix set to abs(ix) if y is integer.  */
      vix = svand_m (yint_or_xpos, vix0, 0x7fffffff);
      /* Set to SignBias if x is negative and y is odd.  */
      sign_bias = svsel (yisodd_xisneg, sv_u32 (d->sign_bias), sv_u32 (0));
    }

  /* Special cases of x or y: zero, inf and nan.  */
  svbool_t xspecial = sv_zeroinfnan (pg, vix0);
  svbool_t yspecial = sv_zeroinfnan (pg, viy0);
  svbool_t cmp = svorr_z (pg, xspecial, yspecial);

  /* Cases of subnormal x: |x| < 0x1p-126.  */
  svbool_t x_is_subnormal = svaclt (yint_or_xpos, x, d->small_bound);
  if (unlikely (svptest_any (yint_or_xpos, x_is_subnormal)))
    {
      /* Normalize subnormal x so exponent becomes negative.  */
      vix = svreinterpret_u32 (svmul_m (x_is_subnormal, x, 0x1p23f));
      vix = svand_m (x_is_subnormal, vix, 0x7fffffff);
      vix = svsub_m (x_is_subnormal, vix, d->subnormal_bias);
    }
  /* Part of core computation carried in working precision.  */
  svuint32_t tmp = svsub_x (yint_or_xpos, vix, d->off);
  svuint32_t i = svand_x (
      yint_or_xpos, svlsr_x (yint_or_xpos, tmp, (23 - V_POWF_LOG2_TABLE_BITS)),
      V_POWF_LOG2_N - 1);
  svuint32_t top = svand_x (yint_or_xpos, tmp, 0xff800000);
  svuint32_t iz = svsub_x (yint_or_xpos, vix, top);
  svint32_t k = svasr_x (yint_or_xpos, svreinterpret_s32 (top),
			 (23 - V_POWF_EXP2_TABLE_BITS));

  /* Compute core in extended precision and return intermediate ylogx results
     to handle cases of underflow and overflow in exp.  */
  svfloat32_t ylogx;
  svfloat32_t ret
      = sv_powf_core (yint_or_xpos, i, iz, k, y, sign_bias, &ylogx, d);

  /* Handle exp special cases of underflow and overflow.  */
  svuint32_t sign
      = svlsl_x (yint_or_xpos, sign_bias, 20 - V_POWF_EXP2_TABLE_BITS);
  svfloat32_t ret_oflow
      = svreinterpret_f32 (svorr_x (yint_or_xpos, sign, asuint (INFINITY)));
  svfloat32_t ret_uflow = svreinterpret_f32 (sign);
  ret = svsel (svcmple (yint_or_xpos, ylogx, d->uflow_bound), ret_uflow, ret);
  ret = svsel (svcmpgt (yint_or_xpos, ylogx, d->oflow_bound), ret_oflow, ret);

  /* Cases of finite y and finite negative x.  */
  ret = svsel (yint_or_xpos, ret, sv_f32 (__builtin_nanf ("")));

  if (unlikely (svptest_any (cmp, cmp)))
    return sv_call_powf_sc (x, y, ret, cmp);

  return ret;
}

TEST_SIG (SV, F, 2, pow)
TEST_ULP (SV_NAME_F2 (pow), 2.08)
/* Wide intervals spanning the whole domain but shared between x and y.  */
#define SV_POWF_INTERVAL2(xlo, xhi, ylo, yhi, n)                              \
  TEST_INTERVAL2 (SV_NAME_F2 (pow), xlo, xhi, ylo, yhi, n)                    \
  TEST_INTERVAL2 (SV_NAME_F2 (pow), xlo, xhi, -ylo, -yhi, n)                  \
  TEST_INTERVAL2 (SV_NAME_F2 (pow), -xlo, -xhi, ylo, yhi, n)                  \
  TEST_INTERVAL2 (SV_NAME_F2 (pow), -xlo, -xhi, -ylo, -yhi, n)
SV_POWF_INTERVAL2 (0, 0x1p-126, 0, inf, 40000)
SV_POWF_INTERVAL2 (0x1p-126, 1, 0, inf, 50000)
SV_POWF_INTERVAL2 (1, inf, 0, inf, 50000)
/* x~1 or y~1.  */
SV_POWF_INTERVAL2 (0x1p-1, 0x1p1, 0x1p-10, 0x1p10, 10000)
SV_POWF_INTERVAL2 (0x1.ep-1, 0x1.1p0, 0x1p8, 0x1p16, 10000)
SV_POWF_INTERVAL2 (0x1p-500, 0x1p500, 0x1p-1, 0x1p1, 10000)
/* around estimated argmaxs of ULP error.  */
SV_POWF_INTERVAL2 (0x1p-300, 0x1p-200, 0x1p-20, 0x1p-10, 10000)
SV_POWF_INTERVAL2 (0x1p50, 0x1p100, 0x1p-20, 0x1p-10, 10000)
/* x is negative, y is odd or even integer, or y is real not integer.  */
TEST_INTERVAL2 (SV_NAME_F2 (pow), -0.0, -10.0, 3.0, 3.0, 10000)
TEST_INTERVAL2 (SV_NAME_F2 (pow), -0.0, -10.0, 4.0, 4.0, 10000)
TEST_INTERVAL2 (SV_NAME_F2 (pow), -0.0, -10.0, 0.0, 10.0, 10000)
TEST_INTERVAL2 (SV_NAME_F2 (pow), 0.0, 10.0, -0.0, -10.0, 10000)
/* |x| is inf, y is odd or even integer, or y is real not integer.  */
SV_POWF_INTERVAL2 (inf, inf, 0.5, 0.5, 1)
SV_POWF_INTERVAL2 (inf, inf, 1.0, 1.0, 1)
SV_POWF_INTERVAL2 (inf, inf, 2.0, 2.0, 1)
SV_POWF_INTERVAL2 (inf, inf, 3.0, 3.0, 1)
/* 0.0^y.  */
SV_POWF_INTERVAL2 (0.0, 0.0, 0.0, 0x1p120, 1000)
/* 1.0^y.  */
TEST_INTERVAL2 (SV_NAME_F2 (pow), 1.0, 1.0, 0.0, 0x1p-50, 1000)
TEST_INTERVAL2 (SV_NAME_F2 (pow), 1.0, 1.0, 0x1p-50, 1.0, 1000)
TEST_INTERVAL2 (SV_NAME_F2 (pow), 1.0, 1.0, 1.0, 0x1p100, 1000)
TEST_INTERVAL2 (SV_NAME_F2 (pow), 1.0, 1.0, -1.0, -0x1p120, 1000)
CLOSE_SVE_ATTR
