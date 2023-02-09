/*
 * Single-precision SVE erfc(x) function.
 *
 * Copyright (c) 2023, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "sv_math.h"
#include "pl_sig.h"
#include "pl_test.h"

#if SV_SUPPORTED

#define P __erfcf_poly_data.poly_T

svfloat64_t SV_NAME_D1 (exp) (svfloat64_t, svbool_t);

static NOINLINE svfloat32_t
special_case (svfloat32_t x, svfloat32_t y, svbool_t special)
{
  return sv_call_f32 (erfcf, x, y, special);
}

static inline svfloat64_t
estrin_lvl1 (svbool_t pg, svfloat64_t x, svuint64_t idx, u64_t i, u64_t j)
{
  svfloat64_t a = svtbl_f64 (svld1_f64 (pg, &P[i][0]), idx);
  svfloat64_t b = svtbl_f64 (svld1_f64 (pg, &P[j][0]), idx);

  if (unlikely (svcntd () < 4))
    {
      /* svcntd may be 2, in which case lanes of a and b where idx == 2 or 3
	 will be zeroed as out-of-range. Correct this.

	 usehi is true for lanes for which the tbl was out-of-range.
	 Load coeffs 2 and 3 to a quad-word, select the right one with tbl and
	 write them back to a and b using usehi.  */
      svbool_t usehi = svcmpge_n_u64 (svptrue_b64 (), idx, 2);
      svuint64_t idx_hi = svsub_n_u64_x (usehi, idx, 2);

      svfloat64_t a_hi
	= svtbl_f64 (svld1rq_f64 (svptrue_b64 (), &P[i][2]), idx_hi);
      svfloat64_t b_hi
	= svtbl_f64 (svld1rq_f64 (svptrue_b64 (), &P[j][2]), idx_hi);

      a = svsel_f64 (usehi, a_hi, a);
      b = svsel_f64 (usehi, b_hi, b);
    }

  return sv_fma_f64_x (pg, x, b, a);
}

static inline svfloat64_t
sv_eval_poly_estrin (svbool_t pg, svfloat64_t z, svuint64_t idx)
{
  svfloat64_t z2 = svmul_f64_x (pg, z, z);
  svfloat64_t z4 = svmul_f64_x (pg, z2, z2);
  svfloat64_t z8 = svmul_f64_x (pg, z4, z4);

  svfloat64_t p_0_1 = estrin_lvl1 (pg, z, idx, 0, 1);
  svfloat64_t p_2_3 = estrin_lvl1 (pg, z, idx, 2, 3);
  svfloat64_t p_4_5 = estrin_lvl1 (pg, z, idx, 4, 5);
  svfloat64_t p_6_7 = estrin_lvl1 (pg, z, idx, 6, 7);
  svfloat64_t p_8_9 = estrin_lvl1 (pg, z, idx, 8, 9);
  svfloat64_t p_10_11 = estrin_lvl1 (pg, z, idx, 10, 11);
  svfloat64_t p_12_13 = estrin_lvl1 (pg, z, idx, 12, 13);
  svfloat64_t p_14_15 = estrin_lvl1 (pg, z, idx, 14, 15);

  svfloat64_t p_0_3 = sv_fma_f64_x (pg, z2, p_2_3, p_0_1);
  svfloat64_t p_4_7 = sv_fma_f64_x (pg, z2, p_6_7, p_4_5);
  svfloat64_t p_8_11 = sv_fma_f64_x (pg, z2, p_10_11, p_8_9);
  svfloat64_t p_12_15 = sv_fma_f64_x (pg, z2, p_14_15, p_12_13);

  svfloat64_t p_0_7 = sv_fma_f64_x (pg, z4, p_4_7, p_0_3);
  svfloat64_t p_8_15 = sv_fma_f64_x (pg, z4, p_12_15, p_8_11);

  svfloat64_t p_0_15 = sv_fma_f64_x (pg, z8, p_8_15, p_0_7);
  return p_0_15;
}

static inline svuint32_t
sv_interval_index (svbool_t pg, svuint32_t ia12)
{
  svuint32_t i_0 = sv_u32 (0);
  svuint32_t i_1 = sv_u32 (1);
  svuint32_t i_2 = sv_u32 (2);
  svuint32_t idx = sv_u32 (3);

  /* 4 intervals with bounds 2.0, 4.0, 8.0 and 10.  */
  idx = svsel_u32 (svcmplt_n_u32 (pg, ia12, 0x410), i_2, idx);
  idx = svsel_u32 (svcmplt_n_u32 (pg, ia12, 0x408), i_1, idx);
  idx = svsel_u32 (svcmplt_n_u32 (pg, ia12, 0x400), i_0, idx);
  return idx;
}

static inline svfloat32_t
sv_approx_erfcf (svfloat32_t abs_x, svuint32_t sign, svuint32_t ia12,
		 svbool_t pg)
{
  /* Input vector has to be split into two as some components need to be
     calculated in double precision. Separate top and bottom halves of input
     into separate vectors and convert to double.  */
  svfloat64_t lo
    = svcvt_f64_f32_x (svptrue_b64 (),
		       sv_as_f32_u64 (svunpklo_u64 (sv_as_u32_f32 (abs_x))));
  svfloat64_t hi
    = svcvt_f64_f32_x (svptrue_b64 (),
		       sv_as_f32_u64 (svunpkhi_u64 (sv_as_u32_f32 (abs_x))));

  /* Have to do the same with pg and the interval indexes.  */
  svbool_t pg_lo = svunpklo_b (pg);
  svbool_t pg_hi = svunpkhi_b (pg);

  svuint32_t interval_idx = sv_interval_index (pg, ia12);
  svuint64_t idx_lo = svunpklo_u64 (interval_idx);
  svuint64_t idx_hi = svunpkhi_u64 (interval_idx);

  /* erfcf(x) ~=     P(x) * exp(-x^2)    : x > 0
		 2 - P(x) * exp(-x^2)    : otherwise.  */
  svfloat64_t poly_lo = sv_eval_poly_estrin (pg_lo, lo, idx_lo);
  svfloat64_t poly_hi = sv_eval_poly_estrin (pg_hi, hi, idx_hi);

  svfloat64_t exp_mx2_lo
    = SV_NAME_D1 (exp) (svneg_f64_x (pg_lo, svmul_f64_x (pg, lo, lo)), pg_lo);
  svfloat64_t exp_mx2_hi
    = SV_NAME_D1 (exp) (svneg_f64_x (pg_hi, svmul_f64_x (pg, hi, hi)), pg_lo);

  lo = svmul_f64_x (pg_lo, poly_lo, exp_mx2_lo);
  hi = svmul_f64_x (pg_hi, poly_hi, exp_mx2_hi);

  /* Convert back to single-precision and interleave.  */
  svfloat32_t lo_32 = svcvt_f32_f64_x (svptrue_b32 (), lo);
  svfloat32_t hi_32 = svcvt_f32_f64_x (svptrue_b32 (), hi);
  svfloat32_t y = svuzp1_f32 (lo_32, hi_32);

  /* Replace y with 2 - y for negative input.  */
  svfloat32_t y_neg = svsubr_n_f32_x (pg, y, 2.0f);
  return svsel_f32 (svcmpeq_n_u32 (pg, sign, 0), y, y_neg);
}

/* Optimized single-precision vector complementary error function
   erfcf. Max measured error: 0.75 ULP. Greatest errors observed between -2^23
   and -2^17, for example:
   SV_NAME_F1 (erfc)(-0x1.6b5a36p-18) got 0x1.000068p+0
			      want 0x1.000066p+0.  */
svfloat32_t SV_NAME_F1 (erfc) (svfloat32_t x, const svbool_t pg)
{
  svuint32_t ix = sv_as_u32_f32 (x);
  svuint32_t ia = svand_n_u32_x (pg, ix, 0x7fffffff);
  svuint32_t ia12 = svlsr_n_u32_x (pg, ia, 20);
  svuint32_t sign = svlsr_n_u32_x (pg, ix, 31);

  /* in_bounds = -4.4 < x < 10.06
	       = |x| < 4.4 || |x| < 10.06 && x < 0).  */
  svbool_t in_bounds
    = svorr_b_z (pg, svcmplt_n_u32 (pg, ia, 0x408ccccd),
		 svand_b_z (pg, svcmpeq_n_u32 (pg, sign, 0),
			    svcmplt_n_u32 (pg, ix, 0x4120f5c3)));

  /* Special cases (nan, inf, -inf) and very small values (|x| < 0x1.0p-26),
     default to scalar implementation which handles these separately.  */
  svbool_t special_cases
    = svcmpge_n_u32 (pg, svsub_n_u32_x (pg, ia12, 0x328), 0x4d0);

  /* erfcf(x) = 0 for x < -4.4
     erfcf(x) = 2 for x > 10.06.  */
  svfloat32_t boring_zone = sv_as_f32_u32 (svlsl_n_u32_x (pg, sign, 30));

  svfloat32_t y
    = svsel_f32 (in_bounds,
		 sv_approx_erfcf (sv_as_f32_u32 (ia), sign, ia12, pg),
		 boring_zone);

  if (unlikely (svptest_any (pg, special_cases)))
    return special_case (x, y, special_cases);

  return y;
}

PL_SIG (SV, F, 1, erfc, -4.0, 10.0)
PL_TEST_ULP (SV_NAME_F1 (erfc), 0.26)
PL_TEST_INTERVAL (SV_NAME_F1 (erfc), 0, 0xffff0000, 10000)
PL_TEST_INTERVAL (SV_NAME_F1 (erfc), 0x1p-127, 0x1p-26, 40000)
PL_TEST_INTERVAL (SV_NAME_F1 (erfc), -0x1p-127, -0x1p-26, 40000)
PL_TEST_INTERVAL (SV_NAME_F1 (erfc), 0x1p-26, 0x1p5, 40000)
PL_TEST_INTERVAL (SV_NAME_F1 (erfc), -0x1p-26, -0x1p3, 40000)
PL_TEST_INTERVAL (SV_NAME_F1 (erfc), 0, inf, 40000)
PL_TEST_INTERVAL (SV_NAME_F1 (erfc), -0, -inf, 40000)

#endif // SV_SUPPORTED
