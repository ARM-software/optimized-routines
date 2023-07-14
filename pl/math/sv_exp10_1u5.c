/*
 * Double-precision SVE 10^x function.
 *
 * Copyright (c) 2023, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "sv_math.h"
#include "pl_sig.h"
#include "pl_test.h"

#define SpecialBound 307.0 /* floor (log10 (2^1023)).  */

static const struct data
{
  double poly[5];
  double shift, log10_2, log2_10_hi, log2_10_lo, scale_thres, special_bound;
} data = {
  /* Coefficients generated using Remez algorithm.
     rel error: 0x1.9fcb9b3p-60
     abs error: 0x1.a20d9598p-60 in [ -log10(2)/128, log10(2)/128 ]
     max ulp err 0.52 +0.5.  */
  .poly = { 0x1.26bb1bbb55516p1, 0x1.53524c73cd32ap1, 0x1.0470591daeafbp1,
	    0x1.2bd77b1361ef6p0, 0x1.142b5d54e9621p-1 },
  /* 1.5*2^46+1023. This value is further explained below.  */
  .shift = 0x1.800000000ffc0p+46,
  .log10_2 = 0x1.a934f0979a371p1,     /* 1/log2(10).  */
  .log2_10_hi = 0x1.34413509f79ffp-2, /* log2(10).  */
  .log2_10_lo = -0x1.9dc1da994fd21p-59,
  .scale_thres = 1280.0,
  .special_bound = SpecialBound,
};

#define C(i) sv_f64 (d->poly[i])
#define SpecialOffset 0x6000000000000000 /* 0x1p513.  */
/* SpecialBias1 + SpecialBias1 = asuint(1.0).  */
#define SpecialBias1 0x7000000000000000 /* 0x1p769.  */
#define SpecialBias2 0x3010000000000000 /* 0x1p-254.  */

/* Update of both special and non-special cases, if any special case is
   detected.  */
static inline svfloat64_t
special_case (svbool_t pg, svfloat64_t s, svfloat64_t y, svfloat64_t n,
	      const struct data *d)
{
  /* s=2^n may overflow, break it up into s=s1*s2,
     such that exp = s + s*y can be computed as s1*(s2+s2*y)
     and s1*s1 overflows only if n>0.  */

  /* If n<=0 then set b to 0x6, 0 otherwise.  */
  svbool_t p_sign = svcmple_n_f64 (pg, n, 0.0); /* n <= 0.  */
  svuint64_t b = svdup_n_u64_z (p_sign, SpecialOffset);

  /* Set s1 to generate overflow depending on sign of exponent n.  */
  svfloat64_t s1
      = svreinterpret_f64_u64 (svsubr_n_u64_x (pg, b, SpecialBias1));
  /* Offset s to avoid overflow in final result if n is below threshold.  */
  svfloat64_t s2 = svreinterpret_f64_u64 (svadd_u64_x (
      pg, svsub_n_u64_x (pg, svreinterpret_u64_f64 (s), SpecialBias2), b));

  /* |n| > 1280 => 2^(n) overflows.  */
  svbool_t p_cmp = svacgt_n_f64 (pg, n, d->scale_thres);

  svfloat64_t r1 = svmul_f64_x (pg, s1, s1);
  svfloat64_t r2 = svmla_f64_x (pg, s2, s2, y);
  svfloat64_t r0 = svmul_f64_x (pg, r2, s1);

  return svsel_f64 (p_cmp, r1, r0);
}

/* Fast vector implementation of exp10 using FEXPA instruction.
   Maximum measured error is 1.02 ulp.
   SV_NAME_D1 (exp10)(-0x1.2862fec805e58p+2) got 0x1.885a89551d782p-16
					    want 0x1.885a89551d781p-16.  */
svfloat64_t SV_NAME_D1 (exp10) (svfloat64_t x, svbool_t pg)
{
  const struct data *d = ptr_barrier (&data);
  svbool_t no_big_scale = svacle_n_f64 (pg, x, d->special_bound);
  svbool_t special = svnot_b_z (pg, no_big_scale);

  /* n = round(x/(log10(2)/N)).  */
  svfloat64_t shift = sv_f64 (d->shift);
  svfloat64_t z = svmla_n_f64_x (pg, shift, x, d->log10_2);
  svfloat64_t n = svsub_f64_x (pg, z, shift);

  /* r = x - n*log10(2)/N.  */
  svfloat64_t log2_10 = svld1rq_f64 (svptrue_b64 (), &d->log2_10_hi);
  svfloat64_t r = x;
  r = svmls_lane_f64 (r, n, log2_10, 0);
  r = svmls_lane_f64 (r, n, log2_10, 1);

  /* scale = 2^(n/N), computed using FEXPA. FEXPA does not propagate NaNs, so
     for consistent NaN handling we have to manually propagate them. This
     comes at significant performance cost.  */
  svuint64_t u = svreinterpret_u64_f64 (z);
  svfloat64_t scale = svexpa_f64 (u);

  /* Approximate exp10(r) using polynomial.  */
  svfloat64_t r2 = svmul_f64_x (pg, r, r);
  svfloat64_t p12 = svmla_f64_x (pg, C (1), r, C (2));
  svfloat64_t p34 = svmla_f64_x (pg, C (3), r, C (4));
  svfloat64_t y = svmla_f64_x (pg, p12, r2, p34);
  y = svmla_f64_x (pg, svmul_f64_x (pg, r, C (0)), r2, y);

  /* Assemble result as exp10(x) = 2^n * exp10(r).  If |x| > SpecialBound
     multiplication may overflow, so use special case routine.  */
  if (unlikely (svptest_any (pg, special)))
    {
      /* FEXPA zeroes the sign bit, however the sign is meaningful to the
	 special case function so needs to be copied.
	 e = sign bit of u << 46.  */
      svuint64_t e
	  = svand_n_u64_x (pg, svlsl_n_u64_x (pg, u, 46), 0x8000000000000000);
      /* Copy sign to scale.  */
      scale = svreinterpret_f64_u64 (
	  svadd_u64_x (pg, e, svreinterpret_u64_f64 (scale)));
      return special_case (pg, scale, y, n, d);
    }

  /* No special case.  */
  return svmla_f64_x (pg, scale, scale, y);
}

PL_SIG (SV, D, 1, exp10, -9.9, 9.9)
PL_TEST_ULP (SV_NAME_D1 (exp10), 0.52)
PL_TEST_INTERVAL (SV_NAME_D1 (exp10), 0, SpecialBound, 10000)
PL_TEST_INTERVAL (SV_NAME_D1 (exp10), SpecialBound, inf, 1000)
PL_TEST_INTERVAL (SV_NAME_D1 (exp10), -0, -SpecialBound, 1000)
PL_TEST_INTERVAL (SV_NAME_D1 (exp10), -SpecialBound, -inf, 1000)
