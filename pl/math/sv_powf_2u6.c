/*
 * Single-precision SVE powf function.
 *
 * Copyright (c) 2023, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "sv_math.h"
#include "pl_sig.h"
#include "pl_test.h"
#if SV_SUPPORTED

/* The following data is used in the SVE pow core computation
   and special case detection.  */
#define Tinvc __sv_powf_log2_data.invc
#define Tlogc __sv_powf_log2_data.logc
#define Texp __sv_powf_exp2_data.tab
#define A(i) sv_f64 (__sv_powf_log2_data.poly[i])
#define C(i) sv_f64 (__sv_powf_exp2_data.poly[i])
/* 2.6 ulp ~ 0.5 + 2^24 (128*Ln2*relerr_log2 + relerr_exp2).  */
#define SignBias (1 << (SV_POWF_EXP2_TABLE_BITS + 11))
#define Shift 0x1.8p52
#define Off 0x3f35d000
#define ExpUnderFlowBound (-150.0 * SV_POWF_EXP2_SCALE)
#define ExpOverFlowBound (128.0 * SV_POWF_EXP2_SCALE)

/* Check if x is an integer.  */
static inline svbool_t
svisint (svbool_t pg, svfloat32_t x)
{
  return svcmpeq_f32 (pg, svrintz_z (pg, x), x);
}

/* Check if x is real not integer valued.  */
static inline svbool_t
svisnotint (svbool_t pg, svfloat32_t x)
{
  return svcmpne_f32 (pg, svrintz_z (pg, x), x);
}

/* Check if x is an odd integer.  */
static inline svbool_t
svisodd (svbool_t pg, svfloat32_t x)
{
  svfloat32_t y = svmul_n_f32_x (pg, x, 0.5f);
  return svisnotint (pg, y);
}

/* Check if zero, inf or nan.  */
static inline svbool_t
sv_zeroinfnan (svbool_t pg, svuint32_t i)
{
  return svcmpge_n_u32 (pg, svsub_n_u32_x (pg, svmul_n_u32_x (pg, i, 2u), 1),
			2u * 0x7f800000 - 1);
}

/* Returns 0 if not int, 1 if odd int, 2 if even int.  The argument is
   the bit representation of a non-zero finite floating-point value.  */
static inline int
checkint (uint32_t iy)
{
  int e = iy >> 23 & 0xff;
  if (e < 0x7f)
    return 0;
  if (e > 0x7f + 23)
    return 2;
  if (iy & ((1 << (0x7f + 23 - e)) - 1))
    return 0;
  if (iy & (1 << (0x7f + 23 - e)))
    return 1;
  return 2;
}

/* Check if zero, inf or nan.  */
static inline int
zeroinfnan (uint32_t ix)
{
  return 2 * ix - 1 >= 2u * 0x7f800000 - 1;
}

/* A scalar subroutine used to fix main power special cases. Similar to the
   preamble of finite_powf except that we do not update ix and sign_bias. This
   is done in the preamble of the SVE powf.  */
static inline float
powf_specialcase (float x, float y, float z)
{
  uint32_t ix = asuint (x);
  uint32_t iy = asuint (y);
  /* Either (x < 0x1p-126 or inf or nan) or (y is 0 or inf or nan).  */
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
      uint32_t sign_bias = 0;
      float_t x2 = x * x;
      if (ix & 0x80000000 && checkint (iy) == 1)
	{
	  x2 = -x2;
	  /* This is only needed if we care about error numbers.  */
	  sign_bias = 1;
	}
      if (2 * ix == 0 && iy & 0x80000000)
	return sign_bias ? -INFINITY : INFINITY;
      /* Without the barrier some versions of clang hoist the 1/x2 and
	 thus division by zero exception can be signaled spuriously.  */
      return iy & 0x80000000 ? opt_barrier_float (1 / x2) : x2;
    }
  /* We need a return here in case x<0 and y is integer, but all other tests
   need to be run.  */
  return z;
}

/* Scalar fallback for special case routines with custom signature.  */
static inline svfloat32_t
sv_call_powf_sc (f32_t (*f) (f32_t, f32_t, f32_t), svfloat32_t x1,
		 svfloat32_t x2, svfloat32_t y, svbool_t cmp)
{
  svbool_t p = svpfirst (cmp, svpfalse ());
  while (svptest_any (cmp, p))
    {
      f32_t sx1 = svclastb_n_f32 (p, 0, x1);
      f32_t sx2 = svclastb_n_f32 (p, 0, x2);
      f32_t elem = svclastb_n_f32 (p, 0, y);
      elem = (*f) (sx1, sx2, elem);
      svfloat32_t y2 = svdup_n_f32 (elem);
      y = svsel_f32 (p, y2, y);
      p = svpnext_b32 (cmp, p);
    }
  return y;
}

/* Compute core for half of the lanes in double precision.  */
static inline svfloat64_t
sv_powf_core_ext (const svbool_t pg, svuint64_t i, svfloat64_t z, svint64_t k,
		  svfloat64_t y, svuint64_t sign_bias, svfloat64_t *pylogx)
{
  svfloat64_t invc = svld1_gather_u64index_f64 (pg, Tinvc, i);
  svfloat64_t logc = svld1_gather_u64index_f64 (pg, Tlogc, i);

  /* log2(x) = log1p(z/c-1)/ln2 + log2(c) + k.  */
  svfloat64_t r = svmla_f64_x (pg, sv_f64 (-1.0), z, invc);
  svfloat64_t y0 = svadd_f64_x (pg, logc, svcvt_f64_s64_x (pg, k));

  /* Polynomial to approximate log1p(r)/ln2.  */
  svfloat64_t logx = A (0);
  logx = svmla_f64_x (pg, A (1), r, logx);
  logx = svmla_f64_x (pg, A (2), r, logx);
  logx = svmla_f64_x (pg, A (3), r, logx);
  logx = svmla_f64_x (pg, y0, r, logx);
  *pylogx = svmul_f64_x (pg, y, logx);

  /* z - kd is in [-1, 1] in non-nearest rounding modes.  */
  svfloat64_t kd = svadd_n_f64_x (pg, *pylogx, Shift);
  svuint64_t ki = svreinterpret_u64_f64 (kd);
  kd = svsub_n_f64_x (pg, kd, Shift);

  r = svsub_f64_x (pg, *pylogx, kd);

  /* exp2(x) = 2^(k/N) * 2^r ~= s * (C0*r^3 + C1*r^2 + C2*r + 1).  */
  svuint64_t t
    = svld1_gather_u64index_u64 (pg, Texp,
				 svand_n_u64_x (pg, ki, SV_POWF_EXP2_N - 1));
  svuint64_t ski = svadd_u64_x (pg, ki, sign_bias);
  t = svadd_u64_x (pg, t,
		   svlsl_n_u64_x (pg, ski, 52 - SV_POWF_EXP2_TABLE_BITS));
  svfloat64_t s = svreinterpret_f64_u64 (t);

  svfloat64_t p = C (0);
  p = svmla_f64_x (pg, C (1), p, r);
  p = svmla_f64_x (pg, C (2), p, r);
  p = svmla_f64_x (pg, s, p, svmul_f64_x (pg, s, r));

  return p;
}

/* Widen vector to double precision and compute core on both halves of the
   vector. Lower cost of promotion by considering all lanes active.  */
static inline svfloat32_t
sv_powf_core (const svbool_t pg, svuint32_t i, svuint32_t iz, svint32_t k,
	      svfloat32_t y, svuint32_t sign_bias, svfloat32_t *pylogx)
{
  const svbool_t ptrue = svptrue_b64 ();

  /* Unpack and promote input vectors (pg, y, z, i, k and sign_bias) into two in
     order to perform core computation in double precision.  */
  const svbool_t pg_lo = svunpklo_b (pg);
  const svbool_t pg_hi = svunpkhi_b (pg);
  svfloat64_t y_lo
    = svcvt_f64_f32_x (ptrue, svreinterpret_f32_u64 (
				svunpklo_u64 (svreinterpret_u32_f32 (y))));
  svfloat64_t y_hi
    = svcvt_f64_f32_x (ptrue, svreinterpret_f32_u64 (
				svunpkhi_u64 (svreinterpret_u32_f32 (y))));
  svfloat32_t z = svreinterpret_f32_u32 (iz);
  svfloat64_t z_lo
    = svcvt_f64_f32_x (ptrue, svreinterpret_f32_u64 (
				svunpklo_u64 (svreinterpret_u32_f32 (z))));
  svfloat64_t z_hi
    = svcvt_f64_f32_x (ptrue, svreinterpret_f32_u64 (
				svunpkhi_u64 (svreinterpret_u32_f32 (z))));
  svuint64_t i_lo = svunpklo_u64 (i);
  svuint64_t i_hi = svunpkhi_u64 (i);
  svint64_t k_lo = svunpklo_s64 (k);
  svint64_t k_hi = svunpkhi_s64 (k);
  svuint64_t sign_bias_lo = svunpklo_u64 (sign_bias);
  svuint64_t sign_bias_hi = svunpkhi_u64 (sign_bias);

  /* Compute each part in double precision.  */
  svfloat64_t ylogx_lo, ylogx_hi;
  svfloat64_t lo
    = sv_powf_core_ext (pg_lo, i_lo, z_lo, k_lo, y_lo, sign_bias_lo, &ylogx_lo);
  svfloat64_t hi
    = sv_powf_core_ext (pg_hi, i_hi, z_hi, k_hi, y_hi, sign_bias_hi, &ylogx_hi);

  /* Convert back to single-precision and interleave.  */
  svfloat32_t ylogx_lo_32 = svcvt_f32_f64_x (ptrue, ylogx_lo);
  svfloat32_t ylogx_hi_32 = svcvt_f32_f64_x (ptrue, ylogx_hi);
  *pylogx = svuzp1_f32 (ylogx_lo_32, ylogx_hi_32);
  svfloat32_t lo_32 = svcvt_f32_f64_x (ptrue, lo);
  svfloat32_t hi_32 = svcvt_f32_f64_x (ptrue, hi);
  return svuzp1_f32 (lo_32, hi_32);
}

/* Implementation of SVE powf.
   Provides the same accuracy as Neon powf, since it relies on the same
   algorithm. The theoretical maximum error is under 2.60 ULPs.
   Maximum measured error is 2.56 ULPs:
   _ZGVsMxvv_powf(0x1.004118p+0, 0x1.5d14a4p+16) got 0x1.fd4bp+127
						want 0x1.fd4b06p+127.  */
svfloat32_t SV_NAME_F2 (pow) (svfloat32_t x, svfloat32_t y, const svbool_t pg)
{
  svuint32_t vix0 = svreinterpret_u32_f32 (x);
  svuint32_t viy0 = svreinterpret_u32_f32 (y);
  svuint32_t vtopx0 = svlsr_n_u32_m (pg, vix0, 20);

  /* Negative x cases.  */
  svuint32_t sign_bit = svand_n_u32_m (pg, vtopx0, 0x800);
  svbool_t xisneg = svcmpeq_n_u32 (pg, sign_bit, 0x800);

  /* Set sign_bias and ix depending on sign of x and nature of y.  */
  svbool_t yisnotint_xisneg = svpfalse_b ();
  svuint32_t sign_bias = svdup_u32 (0);
  svuint32_t vix = vix0;
  if (unlikely (svptest_any (pg, xisneg)))
    {
      /* Determine nature of y.  */
      yisnotint_xisneg = svisnotint (xisneg, y);
      svbool_t yisint_xisneg = svisint (xisneg, y);
      svbool_t yisodd_xisneg = svisodd (xisneg, y);

      /* ix set to abs(ix) if y is integer.  */
      vix = svand_n_u32_m (yisint_xisneg, vix0, 0x7fffffff);
      /* Set to SIGN_BIAS if x is negative and y is odd.  */
      sign_bias
	= svsel_u32 (yisodd_xisneg, svdup_u32 (SignBias), svdup_u32 (0));
    }

  /* Special cases of x or y: zero, inf and nan.  */
  svbool_t xspecial = sv_zeroinfnan (pg, vix0);
  svbool_t yspecial = sv_zeroinfnan (pg, viy0);
  svbool_t cmp = svorr_b_z (pg, xspecial, yspecial);

  /* Small cases of x: |x| < 0x1p-126.  */
  svuint32_t vabstopx0 = svand_n_u32_x (pg, vtopx0, 0x7ff);
  svbool_t xsmall = svcmplt_n_u32 (pg, vabstopx0, 0x008);
  if (unlikely (svptest_any (pg, xsmall)))
    {
      /* Normalize subnormal x so exponent becomes negative.  */
      svuint32_t vix_norm
	= svreinterpret_u32_f32 (svmul_n_f32_x (xsmall, x, 0x1p23f));
      vix_norm = svand_n_u32_x (xsmall, vix_norm, 0x7fffffff);
      vix_norm = svsub_n_u32_x (xsmall, vix_norm, 23 << 23);
      vix = svsel_u32 (xsmall, vix_norm, vix);
    }
  /* Part of core computation carried in working precision.  */
  svuint32_t tmp = svsub_n_u32_x (pg, vix, Off);
  svuint32_t i
    = svand_n_u32_x (pg,
		     svlsr_n_u32_x (pg, tmp, (23 - SV_POWF_LOG2_TABLE_BITS)),
		     SV_POWF_LOG2_N - 1);
  svuint32_t top = svand_n_u32_x (pg, tmp, 0xff800000);
  svuint32_t iz = svsub_u32_x (pg, vix, top);
  svint32_t k = svasr_n_s32_x (pg, svreinterpret_s32_u32 (top),
			       (23 - SV_POWF_EXP2_TABLE_BITS));

  /* Compute core in extended precision and return intermediate ylogx results to
      handle cases of underflow and underflow in exp.  */
  svfloat32_t ylogx;
  svfloat32_t ret = sv_powf_core (pg, i, iz, k, y, sign_bias, &ylogx);

  /* Handle exp special cases of underflow and overflow.  */
  svuint32_t sign = svlsl_n_u32_x (pg, sign_bias, 20 - SV_POWF_EXP2_TABLE_BITS);
  svfloat32_t ret_oflow
    = svreinterpret_f32_u32 (svorr_n_u32_x (pg, sign, asuint (INFINITY)));
  svfloat32_t ret_uflow = svreinterpret_f32_u32 (sign);
  ret
    = svsel_f32 (svcmple_n_f32 (pg, ylogx, ExpUnderFlowBound), ret_uflow, ret);
  ret = svsel_f32 (svcmpgt_n_f32 (pg, ylogx, ExpOverFlowBound), ret_oflow, ret);

  /* Cases of finite y and finite negative x.  */
  ret = svsel_f32 (yisnotint_xisneg, sv_f32 (__builtin_nanf ("")), ret);

  if (unlikely (svptest_any (pg, cmp)))
    return sv_call_powf_sc (powf_specialcase, x, y, ret, cmp);

  return ret;
}

PL_SIG (SV, F, 2, pow)
PL_TEST_ULP (SV_NAME_F2 (pow), 2.06)
/* Wide intervals spanning the whole domain but shared between x and y.  */
#define SV_POWF_INTERVAL(lo, hi, n)                                            \
  PL_TEST_INTERVAL (SV_NAME_F2 (pow), lo, hi, n)                               \
  PL_TEST_INTERVAL (SV_NAME_F2 (pow), -lo, -hi, n)
SV_POWF_INTERVAL (0, 0x1p-126, 40000)
SV_POWF_INTERVAL (0x1p-126, 0x1p-65, 50000)
SV_POWF_INTERVAL (0x1p-65, 1, 50000)
SV_POWF_INTERVAL (1, 0x1p63, 50000)
SV_POWF_INTERVAL (0x1p63, inf, 50000)
SV_POWF_INTERVAL (0, inf, 100000)
/* x~1 or y~1.  */
#define SV_POWF_INTERVAL2(xlo, xhi, ylo, yhi, n)                               \
  PL_TEST_INTERVAL2 (SV_NAME_F2 (pow), xlo, xhi, ylo, yhi, n)                  \
  PL_TEST_INTERVAL2 (SV_NAME_F2 (pow), xlo, xhi, -ylo, -yhi, n)                \
  PL_TEST_INTERVAL2 (SV_NAME_F2 (pow), -xlo, -xhi, ylo, yhi, n)                \
  PL_TEST_INTERVAL2 (SV_NAME_F2 (pow), -xlo, -xhi, -ylo, -yhi, n)
SV_POWF_INTERVAL2 (0x1p-1, 0x1p1, 0x1p-10, 0x1p10, 10000)
SV_POWF_INTERVAL2 (0x1.ep-1, 0x1.1p0, 0x1p8, 0x1p16, 10000)
SV_POWF_INTERVAL2 (0x1p-500, 0x1p500, 0x1p-1, 0x1p1, 10000)
/* around estimated argmaxs of ULP error.  */
SV_POWF_INTERVAL2 (0x1p-300, 0x1p-200, 0x1p-20, 0x1p-10, 10000)
SV_POWF_INTERVAL2 (0x1p50, 0x1p100, 0x1p-20, 0x1p-10, 10000)
/* x is negative, y is odd or even integer, or y is real not integer.  */
PL_TEST_INTERVAL2 (SV_NAME_F2 (pow), -0.0, -10.0, 3.0, 3.0, 10000)
PL_TEST_INTERVAL2 (SV_NAME_F2 (pow), -0.0, -10.0, 4.0, 4.0, 10000)
PL_TEST_INTERVAL2 (SV_NAME_F2 (pow), -0.0, -10.0, 0.0, 10.0, 10000)
PL_TEST_INTERVAL2 (SV_NAME_F2 (pow), 0.0, 10.0, -0.0, -10.0, 10000)
/* |x| is inf, y is odd or even integer, or y is real not integer.  */
SV_POWF_INTERVAL2 (inf, inf, 0.5, 0.5, 1)
SV_POWF_INTERVAL2 (inf, inf, 1.0, 1.0, 1)
SV_POWF_INTERVAL2 (inf, inf, 2.0, 2.0, 1)
SV_POWF_INTERVAL2 (inf, inf, 3.0, 3.0, 1)
/* 0.0^y.  */
SV_POWF_INTERVAL2 (0.0, 0.0, 0.0, 0x1p120, 1000)
/* 1.0^y.  */
PL_TEST_INTERVAL2 (SV_NAME_F2 (pow), 1.0, 1.0, 0.0, 0x1p-50, 1000)
PL_TEST_INTERVAL2 (SV_NAME_F2 (pow), 1.0, 1.0, 0x1p-50, 1.0, 1000)
PL_TEST_INTERVAL2 (SV_NAME_F2 (pow), 1.0, 1.0, 1.0, 0x1p100, 1000)
PL_TEST_INTERVAL2 (SV_NAME_F2 (pow), 1.0, 1.0, -1.0, -0x1p120, 1000)
/* For some NaNs, AOR powf algorithm will get the sign wrong.
   There are plans to relax the requirements on NaNs.  */
#endif
