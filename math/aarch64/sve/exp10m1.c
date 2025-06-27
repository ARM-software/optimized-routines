/*
 * Double-precision vector 10^x - 1 function.
 *
 * Copyright (c) 2025, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "sv_math.h"
#include "test_defs.h"

/* Value of |x| above which scale overflows without special treatment.  */
#define SpecialBound 0x1.33f4bedd4fa70p+8 /* log10(2^(1023 + 1/128)).  */

/* Value of n above which scale overflows even with special treatment.  */
#define ScaleBound 1280.0

/* Value of |x| below which scale - 1 contributes produces large error.  */
#define FexpaBound 0x1.2a9f2b61a7e2p-4 /* 31*(log10(2)/128).  */

static const struct data
{
  double log2_10_hi, log2_10_lo;
  double c3, c5;
  double c0, c1, c2, c4;
  double shift, log10_2, special_bound;
  uint64_t scalem1[32];
} data = {
  /* Coefficients generated using Remez algorithm.  */
  .c0 = 0x1.26bb1bbb55516p1,
  .c1 = 0x1.53524c73cea6ap1,
  .c2 = 0x1.0470591d8bd2ep1,
  .c3 = 0x1.2bd7609dfea43p0,
  .c4 = 0x1.142d0d89058f1p-1,
  .c5 = 0x1.a80cf2ddd513p-3,

  /* 1.5*2^46+1023. This value is further explained below.  */
  .shift = 0x1.800000000ffc0p+46,
  .log10_2 = 0x1.a934f0979a371p1,     /* 1/log2(10).  */
  .log2_10_hi = 0x1.34413509f79ffp-2, /* log2(10).  */
  .log2_10_lo = -0x1.9dc1da994fd21p-59,
  .special_bound = SpecialBound,

  /* Table emulating FEXPA - 1, for values of FEXPA close to 1.
     The table holds values of 2^(i/64) - 1, computed in arbitrary precision.
     The 1st half contains values associated to i=0..+15.
     The 2nd half contains values associated to i=0..-15.  */
  .scalem1 = {
    0x0000000000000000, 0x3f864d1f3bc03077, 0x3f966c34c5615d0f,
    0x3fa0e8a30eb37901, 0x3fa6ab0d9f3121ec, 0x3fac7d865a7a3440,
    0x3fb1301d0125b50a, 0x3fb429aaea92ddfb, 0x3fb72b83c7d517ae,
    0x3fba35beb6fcb754, 0x3fbd4873168b9aa8, 0x3fc031dc431466b2,
    0x3fc1c3d373ab11c3, 0x3fc35a2b2f13e6e9, 0x3fc4f4efa8fef709,
    0x3fc6942d3720185a, 0x0000000000000000, 0xbfc331751ec3a814,
    0xbfc20224341286e4, 0xbfc0cf85bed0f8b7, 0xbfbf332113d56b1f,
    0xbfbcc0768d4175a6, 0xbfba46f918837cb7, 0xbfb7c695afc3b424,
    0xbfb53f391822dbc7, 0xbfb2b0cfe1266bd4, 0xbfb01b466423250a,
    0xbfaafd11874c009e, 0xbfa5b505d5b6f268, 0xbfa05e4119ea5d89,
    0xbf95f134923757f3, 0xbf860f9f985bc9f4,
  },
};

#define SpecialOffset 0x6000000000000000 /* 0x1p513.  */
/* SpecialBias1 + SpecialBias1 = asuint(1.0).  */
#define SpecialBias1 0x7000000000000000 /* 0x1p769.  */
#define SpecialBias2 0x3010000000000000 /* 0x1p-254.  */

static NOINLINE svfloat64_t
special_case (svbool_t pg, svfloat64_t y, svfloat64_t s, svfloat64_t p,
	      svfloat64_t n)
{
  /* s=2^n may overflow, break it up into s=s1*s2,
     such that exp = s + s*y can be computed as s1*(s2+s2*y)
     and s1*s1 overflows only if n>0.  */

  /* If n<=0 then set b to 0x6, 0 otherwise.  */
  svbool_t p_sign = svcmple (pg, n, 0.0); /* n <= 0.  */
  svuint64_t b
      = svdup_u64_z (p_sign, SpecialOffset); /* Inactive lanes set to 0.  */

  /* Set s1 to generate overflow depending on sign of exponent n,
     ie. s1 = 0x70...0 - b.  */
  svfloat64_t s1 = svreinterpret_f64 (svsubr_x (pg, b, SpecialBias1));
  /* Offset s to avoid overflow in final result if n is below threshold.
     ie. s2 = as_u64 (s) - 0x3010...0 + b.  */
  svfloat64_t s2 = svreinterpret_f64 (
      svadd_x (pg, svsub_x (pg, svreinterpret_u64 (s), SpecialBias2), b));

  /* |n| > 1280 => 2^(n) overflows.  */
  svbool_t p_cmp = svacgt (pg, n, ScaleBound);

  svfloat64_t r1 = svmul_x (svptrue_b64 (), s1, s1);
  svfloat64_t r2 = svmla_x (pg, s2, s2, p);
  svfloat64_t r0 = svmul_x (svptrue_b64 (), r2, s1);

  svbool_t is_safe = svacle (pg, n, 1023); /* Only correct special lanes.  */
  return svsel (is_safe, y, svsub_x (pg, svsel (p_cmp, r1, r0), 1.0));
}

/* FEXPA based SVE exp10m1 algorithm.
   Maximum measured error is 2.87 + 0.5 ULP:
   _ZGVsMxv_exp10m1(0x1.64645f11e94c6p-4) got 0x1.c64d54eb7658dp-3
					 want 0x1.c64d54eb7658ap-3.  */
svfloat64_t SV_NAME_D1 (exp10m1) (svfloat64_t x, svbool_t pg)
{
  const struct data *d = ptr_barrier (&data);
  svbool_t special = svacgt (pg, x, d->special_bound);

  /* n = round(x/(log10(2)/N)).  */
  svfloat64_t shift = sv_f64 (d->shift);
  svfloat64_t z = svmla_x (pg, shift, x, d->log10_2);
  svfloat64_t n = svsub_x (pg, z, shift);

  /* r = x - n*log10(2)/N.  */
  svfloat64_t log2_10 = svld1rq (svptrue_b64 (), &d->log2_10_hi);
  svfloat64_t r = x;
  r = svmls_lane (r, n, log2_10, 0);
  r = svmls_lane (r, n, log2_10, 1);

  /* scale = 2^(n/N), computed using FEXPA. FEXPA does not propagate NaNs, so
     for consistent NaN handling we have to manually propagate them. This
     comes at significant performance cost.  */
  svuint64_t u = svreinterpret_u64 (z);
  svfloat64_t scale = svexpa (u);
  svfloat64_t c24 = svld1rq (svptrue_b64 (), &d->c3);
  /* Approximate exp10(r) using polynomial.  */
  svfloat64_t r2 = svmul_x (svptrue_b64 (), r, r);
  svfloat64_t p01 = svmla_x (pg, sv_f64 (d->c0), r, sv_f64 (d->c1));
  svfloat64_t p23 = svmla_lane (sv_f64 (d->c2), r, c24, 0);
  svfloat64_t p45 = svmla_lane (sv_f64 (d->c4), r, c24, 1);
  svfloat64_t p25 = svmla_x (pg, p23, p45, r2);
  svfloat64_t p05 = svmla_x (pg, p01, p25, r2);

  svfloat64_t p = svmul_x (pg, p05, r);

  svfloat64_t scalem1 = svsub_x (pg, scale, 1.0);

  /* For small values, use a lookup table for a more accurate scalem1.  */
  svbool_t is_small = svaclt (pg, x, FexpaBound);
  if (svptest_any (pg, is_small))
    {
      /* Use the low 4 bits of the input of FEXPA as index.  */
      svuint64_t base_idx = svand_x (pg, u, 0xf);

      /* We can use the sign of x as a fifth bit to account for the asymmetry
	 of e^x around 0.  */
      svuint64_t signBit
	  = svlsl_x (pg, svlsr_x (pg, svreinterpret_u64 (x), 63), 4);
      svuint64_t idx = svorr_x (pg, base_idx, signBit);

      /* Lookup values for scale - 1 for small x.  */
      svfloat64_t lookup
	  = svreinterpret_f64 (svld1_gather_index (is_small, d->scalem1, idx));

      /* Select the appropriate scale - 1 value based on x.  */
      scalem1 = svsel (is_small, lookup, scalem1);
    }

  svfloat64_t y = svmla_x (pg, scalem1, scale, p);

  /* FEXPA returns nan for large inputs so we special case those.  */
  if (unlikely (svptest_any (pg, special)))
    {
      /* FEXPA zeroes the sign bit, however the sign is meaningful to the
	  special case function so needs to be copied.
	  e = sign bit of u << 46.  */
      svuint64_t e = svand_x (pg, svlsl_x (pg, u, 46), 0x8000000000000000);
      /* Copy sign to scale.  */
      scale = svreinterpret_f64 (svadd_x (pg, e, svreinterpret_u64 (scale)));
      return special_case (pg, y, scale, p, n);
    }

  return y;
}

#if WANT_C23_TESTS
TEST_ULP (SV_NAME_D1 (exp10m1), 2.87)
TEST_DISABLE_FENV (SV_NAME_D1 (exp10m1))
TEST_INTERVAL (SV_NAME_D1 (exp10m1), 0, 0xffff0000, 10000)
TEST_SYM_INTERVAL (SV_NAME_D1 (exp10m1), 0, FexpaBound, 10000)
TEST_SYM_INTERVAL (SV_NAME_D1 (exp10m1), FexpaBound, SpecialBound, 10000)
TEST_SYM_INTERVAL (SV_NAME_D1 (exp10m1), SpecialBound, inf, 10000)
#endif
CLOSE_SVE_ATTR
