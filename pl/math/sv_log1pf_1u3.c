/*
 * Single-precision vector log(x + 1) function.
 *
 * Copyright (c) 2023, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "sv_math.h"
#include "pl_sig.h"
#include "pl_test.h"
#include "sv_estrinf.h"

#if SV_SUPPORTED

static struct sv_log1pf_data
{
  float poly[8];
  float ln2, exp_bias;
  uint32_t four, three_quarters;
} data = {.poly = {/* Do not store first term of polynomial, which is -0.5, as
                      this can be fmov-ed directly instead of including it in
                      the main load-and-mla polynomial schedule.  */
		   0x1.5555aap-2f, -0x1.000038p-2f, 0x1.99675cp-3f,
		   -0x1.54ef78p-3f, 0x1.28a1f4p-3f, -0x1.0da91p-3f,
		   0x1.abcb6p-4f, -0x1.6f0d5ep-5f},
	  .ln2 = 0x1.62e43p-1f,
	  .exp_bias = 0x1p-23f,
	  .four = 0x40800000,
	  .three_quarters = 0x3f400000};

#define SignExponentMask 0xff800000
#define C(i) sv_f32 (data.poly[i])

static svfloat32_t NOINLINE
special_case (svfloat32_t x, svfloat32_t y, svbool_t special)
{
  return sv_call_f32 (log1pf, x, y, special);
}

/* Vector log1pf approximation using polynomial on reduced interval. Worst-case
   error is 1.27 ULP very close to 0.5.
   _ZGVsMxv_log1pf(0x1.fffffep-2) got 0x1.9f324p-2
				 want 0x1.9f323ep-2.  */
svfloat32_t SV_NAME_F1 (log1p) (svfloat32_t x, svbool_t pg)
{
  /* x < -1, Inf/Nan and -0 (need to catch -0 as argument reduction otherwise
     discards sign).  */
  svbool_t special
    = svorr_b_z (pg, svcmpeq_n_f32 (pg, x, 0),
		 svcmpeq_n_u32 (pg, svreinterpret_u32_f32 (x), 0x7f800000));
  special = svorn_b_z (pg, special, svcmpge_n_f32 (pg, x, -1));

  /* With x + 1 = t * 2^k (where t = m + 1 and k is chosen such that m
			   is in [-0.25, 0.5]):
     log1p(x) = log(t) + log(2^k) = log1p(m) + k*log(2).

     We approximate log1p(m) with a polynomial, then scale by
     k*log(2). Instead of doing this directly, we use an intermediate
     scale factor s = 4*k*log(2) to ensure the scale is representable
     as a normalised fp32 number.  */
  svfloat32_t m = svadd_n_f32_x (pg, x, 1);

  /* Choose k to scale x to the range [-1/4, 1/2].  */
  svint32_t k = svand_s32_x (pg,
			     svsub_n_s32_x (pg, svreinterpret_s32_f32 (m),
					    data.three_quarters),
			     sv_s32 (SignExponentMask));

  /* Scale x by exponent manipulation.  */
  svfloat32_t m_scale = svreinterpret_f32_u32 (
    svsub_u32_x (pg, svreinterpret_u32_f32 (x), svreinterpret_u32_s32 (k)));

  /* Scale up to ensure that the scale factor is representable as normalised
     fp32 number, and scale m down accordingly.  */
  svfloat32_t s = svreinterpret_f32_s32 (svsubr_n_s32_x (pg, k, data.four));
  m_scale = svadd_f32_x (pg, m_scale, svmla_n_f32_x (pg, sv_f32 (-1), s, 0.25));

  /* Evaluate polynomial on reduced interval.  */
  svfloat32_t ms2 = svmul_f32_x (pg, m_scale, m_scale),
	      ms4 = svmul_f32_x (pg, ms2, ms2);
  svfloat32_t p = ESTRIN_7 (pg, m_scale, ms2, ms4, C);
  p = svmad_n_f32_x (pg, m_scale, p, -0.5);
  p = svmla_f32_x (pg, m_scale, m_scale, svmul_f32_x (pg, m_scale, p));

  /* The scale factor to be applied back at the end - by multiplying float(k)
     by 2^-23 we get the unbiased exponent of k.  */
  svfloat32_t scale_back
    = svmul_n_f32_x (pg, svcvt_f32_s32_x (pg, k), data.exp_bias);

  /* Apply the scaling back.  */
  svfloat32_t y = svmla_n_f32_x (pg, p, scale_back, data.ln2);

  if (unlikely (svptest_any (pg, special)))
    return special_case (x, y, special);

  return y;
}

PL_SIG (SV, F, 1, log1p, -0.9, 10.0)
PL_TEST_ULP (SV_NAME_F1 (log1p), 0.77)
PL_TEST_INTERVAL (SV_NAME_F1 (log1p), 0, 0x1p-23, 5000)
PL_TEST_INTERVAL (SV_NAME_F1 (log1p), -0, -0x1p-23, 5000)
PL_TEST_INTERVAL (SV_NAME_F1 (log1p), 0x1p-23, 1, 5000)
PL_TEST_INTERVAL (SV_NAME_F1 (log1p), -0x1p-23, -1, 5000)
PL_TEST_INTERVAL (SV_NAME_F1 (log1p), 1, inf, 10000)
PL_TEST_INTERVAL (SV_NAME_F1 (log1p), -1, -inf, 10)
#endif
