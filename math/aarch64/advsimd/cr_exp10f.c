/*
 * Single-precision correctly rounded vector 10^x function.
 *
 * Copyright (c) 2026, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "v_math.h"
#include "test_defs.h"
#include "test_sig.h"
#include "cr_expf_utils.h"

static const struct data
{
  double c1, c3;
  double log2_10, neg_ln2;
  float64x2_t c0, c2;
  float64x2_t shift;
  float32x4_t range_val, inf;
  struct cr_expf_data exp_data;
} exp10f_data = {
  .c0 = V2 (0x1.fffffffffdbcep-2),
  .c1 = 0x1.55555555543c2p-3,
  .c2 = V2 (0x1.555a7c43cc319p-5),
  .c3 = 0x1.2316e2b693656p-7,
  .shift = V2 (0x1.800000000ffc0p+46),
  .log2_10 = 0x1.a934f0979a371p+1,
  .neg_ln2 = -0x1.62e42fefa3cefp-1,
  .range_val = V4 (0x1p+7),
  .inf = V4 (INFINITY),
  .exp_data = CR_EXPF_DATA,
};

static inline float64x2_t VPCS_ATTR
inline_exp10 (float64x2_t x, const struct data *d)
{
  /* By using 10^x = e^(x * ln(10)) = e^(x * log2(10) * ln(2)).
     By splitting (x * log2(10)) into n + (x*log2(10) - n), where n is
     x*log2(10) rounded to a multiple of 1/64. From this, we then get:
     r = (x * log2(10) - n) * ln(2), with |r| < ln2/128,
     10^x = 2^n * exp(r).  */
  float64x2_t log2_10_negln2 = vld1q_f64 (&d->log2_10);
  float64x2_t z = vfmaq_laneq_f64 (d->shift, x, log2_10_negln2, 0);
  float64x2_t n = vsubq_f64 (z, d->shift);

  float64x2_t r;
  r = vfmsq_laneq_f64 (n, x, log2_10_negln2, 0);
  r = vmulq_laneq_f64 (r, log2_10_negln2, 1);

  float64x2_t coeffs = vld1q_f64 (&d->c1);

  /* poly(r) = exp(r) - 1 ~= r + c0*r^2 + c1*r^3 + c2*r^4 + c3*r^5.  */
  float64x2_t r2 = vmulq_f64 (r, r);
  float64x2_t p01 = vfmaq_laneq_f64 (d->c0, r, coeffs, 0);
  float64x2_t p23 = vfmaq_laneq_f64 (d->c2, r, coeffs, 1);
  float64x2_t p04 = vfmaq_f64 (p01, r2, p23);
  float64x2_t y = vfmaq_f64 (r, r2, p04);

  uint64x2_t u = vreinterpretq_u64_f64 (z);
  float64x2_t s = exp_lookup (u, &d->exp_data);
  return vfmaq_f64 (s, s, y);
}

/* Single-precision correctly rounded vector exp10f routine.  */
float32x4_t VPCS_ATTR NOINLINE V_NAME_F1 (cr_exp10) (float32x4_t x)
{
  const struct data *d = ptr_barrier (&exp10f_data);

  /* Split into upper and lower halves for double-precision computation.  */
  float64x2_t x_d_lo = vcvt_f64_f32 (vget_low_f32 (x));
  float64x2_t x_d_hi = vcvt_high_f64_f32 (x);

  float64x2_t y_lo = inline_exp10 (x_d_lo, d);
  float64x2_t y_hi = inline_exp10 (x_d_hi, d);

  /* Round once to single precision and recombine the results.  */
  float32x4_t ret = vcombine_f32 (vcvt_f32_f64 (y_lo), vcvt_f32_f64 (y_hi));

  /* Saturate inputs outside the range needed by binary32.  */
  uint32x4_t special = vcagtq_f32 (x, d->range_val);
  if (unlikely (v_any_u32 (special)))
    {
      uint32x4_t is_inf = vcgtzq_f32 (x);
      uint32x4_t inf_or_zero
	  = vandq_u32 (is_inf, vreinterpretq_u32_f32 (d->inf));
      float32x4_t special_res = vreinterpretq_f32_u32 (inf_or_zero);
      return vbslq_f32 (special, special_res, ret);
    }

  return ret;
}

HALF_WIDTH_ALIAS_F1 (cr_exp10)

#if WANT_EXP10_TESTS
TEST_SIG (V, F, 1, cr_exp10, -9.9, 9.9)
TEST_ULP (V_NAME_F1 (cr_exp10), 0.00)
TEST_INTERVAL (V_NAME_F1 (cr_exp10), 0, 0xffff0000, 10000)
TEST_SYM_INTERVAL (V_NAME_F1 (cr_exp10), 0, 0x1p-23, 50000)
TEST_SYM_INTERVAL (V_NAME_F1 (cr_exp10), 0x1p-23, 0x1p5, 50000)
TEST_SYM_INTERVAL (V_NAME_F1 (cr_exp10), 0x1p5, 0x1p7, 50000)
TEST_SYM_INTERVAL (V_NAME_F1 (cr_exp10), 0x1p7, inf, 50000)
#endif
