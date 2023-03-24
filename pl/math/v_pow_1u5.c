/*
 * Double-precision vector pow function.
 *
 * Copyright (c) 2020-2023, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "v_math.h"
#include "pl_sig.h"
#include "pl_test.h"

/* Defines parameters of the approximation and scalar fallback.  */
#include "finite_pow.h"

/* This version implements an algorithm close to AOR scalar pow but
   - does not implement the trick in the exp's specialcase subroutine to avoid
     double-rounding,
   - does not use a tail in the exponential core computation,
   - and pow's exp polynomial order and table bits might differ.

   Maximum measured error is 1.04 ULPs:
   _ZGVnN2vv_pow(0x1.024a3e56b3c3p-136, 0x1.87910248b58acp-13)
     got 0x1.f71162f473251p-1
    want 0x1.f71162f473252p-1.  */

/* Compute y+TAIL = log(x) where the rounded result is y and TAIL has about
   additional 15 bits precision.  IX is the bit representation of x, but
   normalized in the subnormal range using the sign bit for the exponent.  */
static inline float64x2_t
v_log_inline (uint64x2_t ix, float64x2_t *tail)
{
  /* x = 2^k z; where z is in range [OFF,2*OFF) and exact.
     The range is split into N subintervals.
     The ith subinterval contains z and c is near its center.  */
  uint64x2_t tmp = ix - v_u64 (OFF);
  uint64x2_t i = ((tmp >> (52 - V_POW_LOG_TABLE_BITS)) & (N_LOG - 1));
  int64x2_t k = vreinterpretq_s64_u64 (tmp) >> 52; /* arithmetic shift.  */
  uint64x2_t iz = ix - (tmp & 0xfffULL << 52);
  float64x2_t z = vreinterpretq_f64_u64 (iz);
  float64x2_t kd = (float64x2_t){k[0], k[1]};
  /* log(x) = k*Ln2 + log(c) + log1p(z/c-1).  */
  float64x2_t invc = v_lookup_f64 (INVC, i);
  float64x2_t logc = v_lookup_f64 (LOGC, i);
  float64x2_t logctail = v_lookup_f64 (LOGCTAIL, i);
  /* Note: 1/c is j/N or j/N/2 where j is an integer in [N,2N) and
     |z/c - 1| < 1/N, so r = z/c - 1 is exactly representible.  */
  float64x2_t r = vfmaq_f64 (v_f64 (-1.0), z, invc);
  /* k*Ln2 + log(c) + r.  */
  float64x2_t t1 = vfmaq_f64 (logc, kd, v_f64 (Ln2hi));
  float64x2_t t2 = t1 + r;
  float64x2_t lo1 = vfmaq_f64 (logctail, kd, v_f64 (Ln2lo));
  float64x2_t lo2 = t1 - t2 + r;
  /* Evaluation is optimized assuming superscalar pipelined execution.  */
  float64x2_t ar = A[0] * r; /* A[0] = -0.5.  */
  float64x2_t ar2 = r * ar;
  float64x2_t ar3 = r * ar2;
  /* k*Ln2 + log(c) + r + A[0]*r*r.  */
  float64x2_t hi = t2 + ar2;
  float64x2_t lo3 = vfmaq_f64 (-ar2, ar, r);
  float64x2_t lo4 = t2 - hi + ar2;
  /* p = log1p(r) - r - A[0]*r*r.  */
  /* log1p(r) = (ar3
	 * (A[1] + r * A[2] + ar2 * (A[3] + r * A[4] + ar2 * (A[5] + r *
	 A[6])))).  */
  float64x2_t a56 = vfmaq_f64 (v_f64 (A[5]), r, v_f64 (A[6]));
  float64x2_t a34 = vfmaq_f64 (v_f64 (A[3]), r, v_f64 (A[4]));
  float64x2_t a12 = vfmaq_f64 (v_f64 (A[1]), r, v_f64 (A[2]));
  float64x2_t p = vfmaq_f64 (a34, ar2, a56);
  p = vfmaq_f64 (a12, ar2, p);
  p = ar3 * p;
  float64x2_t lo = lo1 + lo2 + lo3 + lo4 + p;
  float64x2_t y = hi + lo;
  *tail = hi - y + lo;
  return y;
}

/* Computes exp(x+xtail) where |xtail| < 2^-8/N and |xtail| <= |x|.  */
static NOINLINE double
exp_inline_nosignbias (double x, double xtail)
{
  uint32_t abstop = top12 (x) & 0x7ff;
  if (unlikely (abstop - top12 (0x1p-54) >= top12 (512.0) - top12 (0x1p-54)))
    {
      /* Avoid spurious underflow for tiny x.  */
      if (abstop - top12 (0x1p-54) >= 0x80000000)
	return 1.0;
      /* Note: inf and nan are already handled.  */
      if (abstop >= top12 (1024.0))
#if WANT_SIMD_EXCEPT
	return asuint64 (x) >> 63 ? __math_uflow (0) : __math_oflow (0);
#else
	return asuint64 (x) >> 63 ? 0.0 : INFINITY;
#endif
      /* Large x is special cased below.  */
      abstop = 0;
    }

  /* exp(x) = 2^(k/N) * exp(r), with exp(r) in [2^(-1/2N),2^(1/2N)].  */
  /* x = ln2/N*k + r, with k integer and r in [-ln2/2N, ln2/2N].  */
  double z = InvLn2N * x;
  double kd = roundtoint (z);
  uint64_t ki = converttoint (z);
  double r = x + kd * NegLn2hiN + kd * NegLn2loN;
  /* The code assumes 2^-200 < |xtail| < 2^-8/N.  */
  r += xtail;
  /* 2^(k/N) ~= scale.  */
  uint64_t idx = ki & (N_EXP - 1);
  uint64_t top = ki << (52 - V_POW_EXP_TABLE_BITS);
  /* This is only a valid scale when -1023*N < k < 1024*N.  */
  uint64_t sbits = SBits[idx] + top;
  /* exp(x) = 2^(k/N) * exp(r) ~= scale + scale * (tail + exp(r) - 1).  */
  double r2 = r * r;
  double tmp = r + r2 * C2 + r * r2 * (C3 + r * C4);
  if (unlikely (abstop == 0))
    return specialcase (tmp, sbits, ki);
  double scale = asdouble (sbits);
  /* Note: tmp == 0 or |tmp| > 2^-200 and scale > 2^-739, so there
     is no spurious underflow here even without fma.  */
  return eval_as_double (scale + scale * tmp);
}

/* Computes sign*exp(x+xtail) where |xtail| < 2^-8/N and |xtail| <= |x|.  */
static inline float64x2_t
v_exp_inline (float64x2_t x, float64x2_t xtail)
{
  /* Fallback to scalar exp_inline for all lanes if any lane
     contains value of x s.t. |x| <= 2^-54 or >= 512.  */
  uint64x2_t abstop = (vreinterpretq_u64_f64 (x) >> 52) & v_u64 (0x7ff);
  uint64x2_t uoflowx = (abstop - v_u64 (top12 (0x1p-54))
			>= v_u64 (top12 (512.0)) - v_u64 (top12 (0x1p-54)));
  if (unlikely (v_any_u64 (uoflowx)))
    return v_call2_f64 (exp_inline_nosignbias, x, xtail, x, v_u64 (-1));
  /* exp(x) = 2^(k/N) * exp(r), with exp(r) in [2^(-1/2N),2^(1/2N)].  */
  /* x = ln2/N*k + r, with k integer and r in [-ln2/2N, ln2/2N].  */
  float64x2_t z = v_f64 (InvLn2N) * x;
  /* z - kd is in [-1, 1] in non-nearest rounding modes.  */
  float64x2_t kd = z + v_f64 (Shift);
  uint64x2_t ki = vreinterpretq_u64_f64 (kd);
  kd = kd - v_f64 (Shift);
  float64x2_t r = vfmaq_f64 (x, kd, v_f64 (NegLn2hiN));
  r = vfmaq_f64 (r, kd, v_f64 (NegLn2loN));
  /* The code assumes 2^-200 < |xtail| < 2^-8/N.  */
  r = r + xtail;
  /* 2^(k/N) ~= scale.  */
  uint64x2_t idx = (ki & v_u64 (N_EXP - 1));
  uint64x2_t top = ki << (52 - V_POW_EXP_TABLE_BITS);
  /* This is only a valid scale when -1023*N < k < 1024*N.  */
  uint64x2_t sbits = v_lookup_u64 (SBits, idx);
  sbits = sbits + top;
  /* exp(x) = 2^(k/N) * exp(r) ~= scale + scale * (exp(r) - 1).  */
  float64x2_t r2 = r * r;
  /* tmp = r + r2 * C2 + r * r2 * (C3 + r * C4).  */
  float64x2_t tmp = vfmaq_f64 (v_f64 (C3), r, v_f64 (C4));
  tmp = vfmaq_f64 (v_f64 (C2), r, tmp);
  tmp = vfmaq_f64 (r, r2, tmp);
  float64x2_t scale = vreinterpretq_f64_u64 (sbits);
  /* Note: tmp == 0 or |tmp| > 2^-200 and scale > 2^-739, so there
     is no spurious underflow here even without fma.  */
  return vfmaq_f64 (scale, scale, tmp);
}

VPCS_ATTR
float64x2_t V_NAME_D2 (pow) (float64x2_t x, float64x2_t y)
{
  /* Case of x <= 0 is too complicated to be vectorised efficiently here,
     fallback to scalar pow for all lanes if any x < 0 detected.  */
#if WANT_SIMD_EXCEPT
  uint64x2_t vix = vreinterpretq_u64_f64 (x);
  if (v_any_u64 (vix >> 63) || v_any_u64 (vix == 0))
    return v_call2_f64 (__pl_finite_pow, x, y, x, v_u64 (-1));
#else
  if (v_any_u64 (x <= 0.0))
    return v_call2_f64 (__pl_finite_pow, x, y, x, v_u64 (-1));
  uint64x2_t vix = vreinterpretq_u64_f64 (x);
#endif

  uint64x2_t viy = vreinterpretq_u64_f64 (y);
  uint64x2_t vtopx = vix >> 52;
  uint64x2_t vtopy = viy >> 52;
  uint64x2_t vabstopx = vtopx & 0x7ff;
  uint64x2_t vabstopy = vtopy & 0x7ff;

  /* Special cases of x or y.  */
#if WANT_SIMD_EXCEPT
  /* Small or large.  */
  uint64x2_t specialx = vtopx - 0x001 >= 0x7ff - 0x001;
  uint64x2_t specialy = vabstopy - 0x3be >= 0x43e - 0x3be;
#else
  /* Inf or nan.  */
  uint64x2_t specialx = vabstopx >= 0x7ff;
  uint64x2_t specialy = vabstopy >= 0x7ff;
  /* The case y==0 does not trigger a special case, since in this case it is
     necessary to fix the result only if x is a signalling nan, which already
     triggers a special case. We test y==0 directly in the scalar fallback.  */
#endif
  uint64x2_t special = specialx | specialy;
  /* Fallback to scalar on all lanes if any lane is inf or nan.  */
  if (unlikely (v_any_u64 (special)))
    return v_call2_f64 (__pl_finite_pow, x, y, x, v_u64 (-1));

  /* Small cases of x: |x| < 0x1p-126.  */
  uint64x2_t smallx = vabstopx < 0x001;
  if (unlikely (v_any_u64 (smallx)))
    {
      /* Update ix if top 12 bits of x are 0.  */
      uint64x2_t sub_x = vtopx == 0;
      if (unlikely (v_any_u64 (sub_x)))
	{
	  /* Normalize subnormal x so exponent becomes negative.  */
	  uint64x2_t vix_norm = vreinterpretq_u64_f64 (x * 0x1p52);
	  vix_norm &= 0x7fffffffffffffff;
	  vix_norm -= 52ULL << 52;
	  vix = vbslq_u64 (sub_x, vix_norm, vix);
	}
    }

  /* Vector Log(ix, &lo).  */
  float64x2_t vlo;
  float64x2_t vhi = v_log_inline (vix, &vlo);

  /* Vector Exp(y_loghi, y_loglo).  */
  float64x2_t vehi = y * vhi;
  float64x2_t velo = y * vlo;
  float64x2_t vemi = vfmaq_f64 (-vehi, y, vhi);
  velo = velo + vemi;
  return v_exp_inline (vehi, velo);
}

PL_SIG (V, D, 2, pow)
PL_TEST_ULP (V_NAME_D2 (pow), 0.55)
PL_TEST_EXPECT_FENV (V_NAME_D2 (pow), WANT_SIMD_EXCEPT)
/* Wide intervals spanning the whole domain but shared between x and y.  */
#define V_POW_INTERVAL(lo, hi, n)                                              \
  PL_TEST_INTERVAL (V_NAME_D2 (pow), lo, hi, n)                                \
  PL_TEST_INTERVAL (V_NAME_D2 (pow), -lo, -hi, n)
V_POW_INTERVAL (0, 0x1p-126, 40000)
V_POW_INTERVAL (0x1p-126, 0x1p-65, 50000)
V_POW_INTERVAL (0x1p-65, 1, 50000)
V_POW_INTERVAL (1, 0x1p63, 50000)
V_POW_INTERVAL (0x1p63, inf, 1000)
V_POW_INTERVAL (0, inf, 1000)
/* x~1 or y~1.  */
#define V_POW_INTERVAL2(xlo, xhi, ylo, yhi, n)                                 \
  PL_TEST_INTERVAL2 (V_NAME_D2 (pow), xlo, xhi, ylo, yhi, n)                   \
  PL_TEST_INTERVAL2 (V_NAME_D2 (pow), xlo, xhi, -ylo, -yhi, n)                 \
  PL_TEST_INTERVAL2 (V_NAME_D2 (pow), -xlo, -xhi, ylo, yhi, n)                 \
  PL_TEST_INTERVAL2 (V_NAME_D2 (pow), -xlo, -xhi, -ylo, -yhi, n)
V_POW_INTERVAL2 (0x1p-1, 0x1p1, 0x1p-10, 0x1p10, 10000)
V_POW_INTERVAL2 (0x1p-500, 0x1p500, 0x1p-1, 0x1p1, 10000)
V_POW_INTERVAL2 (0x1.ep-1, 0x1.1p0, 0x1p8, 0x1p16, 10000)
/* around argmaxs of ULP error.  */
V_POW_INTERVAL2 (0x1p-300, 0x1p-200, 0x1p-20, 0x1p-10, 10000)
V_POW_INTERVAL2 (0x1p50, 0x1p100, 0x1p-20, 0x1p-10, 10000)
/* x is negative, y is odd or even integer, or y is real not integer.  */
PL_TEST_INTERVAL2 (V_NAME_D2 (pow), -0.0, -10.0, 3.0, 3.0, 10000)
PL_TEST_INTERVAL2 (V_NAME_D2 (pow), -0.0, -10.0, 4.0, 4.0, 10000)
PL_TEST_INTERVAL2 (V_NAME_D2 (pow), -0.0, -10.0, 0.0, 10.0, 10000)
PL_TEST_INTERVAL2 (V_NAME_D2 (pow), 0.0, 10.0, -0.0, -10.0, 10000)
/* 1.0^y.  */
PL_TEST_INTERVAL2 (V_NAME_D2 (pow), 1.0, 1.0, 0.0, 0x1p-50, 1000)
PL_TEST_INTERVAL2 (V_NAME_D2 (pow), 1.0, 1.0, 0x1p-50, 1.0, 1000)
PL_TEST_INTERVAL2 (V_NAME_D2 (pow), 1.0, 1.0, 1.0, 0x1p100, 1000)
PL_TEST_INTERVAL2 (V_NAME_D2 (pow), 1.0, 1.0, -1.0, -0x1p120, 1000)
/* nan^-nan.  */
PL_TEST_INTERVAL2 (V_NAME_D2 (pow), nan, nan, -nan, -nan, 1)
/* For some NaNs, AOR pow algorithm will get the sign of nan^-nan wrong.
   There are plans to relax the requirements on NaNs.  */
