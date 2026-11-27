/*
 * Single-precision correctly rounded vector e^x function.
 *
 * Copyright (c) 2026, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "v_math.h"
#include "test_defs.h"
#include "test_sig.h"
#include "expf_utils.h"

static const struct data
{
  double c1, c3;
  float64x2_t c0, c2;
  struct cr_expf_data exp_data;
  float64x2_t shift, inv_ln2;
  double ln2_hi, ln2_lo;
  float32x4_t range_val;
  uint32x4_t inf;
} expf_data = {
  .c0 = V2 (0x1.fffffffffdbcep-2),
  .c1 = 0x1.55555555543c2p-3,
  .c2 = V2 (0x1.555573c64f2e3p-5),
  .c3 = 0x1.111126b4eff73p-7,
  .exp_data = CR_EXPF_DATA,
  .shift = V2 (0x1.800000000ffc0p+46),
  .inv_ln2 = V2 (0x1.71547652b82fep+0),
  .ln2_hi = 0x1.62e42fefa39efp-1,
  .ln2_lo = 0x1.abc9e3b39803fp-56,
  .range_val = V4 (0x1p+9),
  .inf = V4 (0x7f800000),
};

static inline float64x2_t VPCS_ATTR
inline_exp (float64x2_t x, const struct data *d)
{
  float64x2_t z = vfmaq_f64 (d->shift, x, d->inv_ln2);
  float64x2_t n = vsubq_f64 (z, d->shift);

  float64x2_t ln2 = vld1q_f64 (&d->ln2_hi);

  float64x2_t r = x;
  r = vfmsq_laneq_f64 (r, n, ln2, 0);
  r = vfmsq_laneq_f64 (r, n, ln2, 1);

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

/* Single-precision correctly rounded vector expf routine.  */
float32x4_t VPCS_ATTR NOINLINE V_NAME_F1 (cr_exp) (float32x4_t x)
{
  const struct data *d = ptr_barrier (&expf_data);

  /* Splits into an upper and lower half for double-precision computation.  */
  float64x2_t x_d_lo = vcvt_f64_f32 (vget_low_f32 (x));
  float64x2_t x_d_hi = vcvt_high_f64_f32 (x);

  /* Compute the double precision exponential for the high and low halves.  */
  float64x2_t y_lo = inline_exp (x_d_lo, d);
  float64x2_t y_hi = inline_exp (x_d_hi, d);

  /* Round to single precision, and recombine the results.  */
  float32x4_t ret = vcombine_f32 (vcvt_f32_f64 (y_lo), vcvt_f32_f64 (y_hi));

  /* Handle special cases: overflow, underflow, and NaNs.  */
  uint32x4_t special = vcagtq_f32 (x, d->range_val);
  if (unlikely (v_any_u32 (special)))
    {
      uint32x4_t is_inf = vcgtzq_f32 (x);
      uint32x4_t inf_or_zero = vandq_u32 (is_inf, d->inf);
      float32x4_t special_res = vreinterpretq_f32_u32 (inf_or_zero);

      /* Combine the results for normal and special cases and return.  */
      return vbslq_f32 (special, special_res, ret);
    }

  return ret;
}

HALF_WIDTH_ALIAS_F1 (cr_exp)

TEST_SIG (V, F, 1, cr_exp, -9.9, 9.9)
TEST_ULP (V_NAME_F1 (cr_exp), 0.00)
TEST_INTERVAL (V_NAME_F1 (cr_exp), 0, 0xffff0000, 10000)
TEST_SYM_INTERVAL (V_NAME_F1 (cr_exp), 0, 0x1p-23, 50000)
TEST_SYM_INTERVAL (V_NAME_F1 (cr_exp), 0x1p-23, 0x1p9, 50000)
TEST_SYM_INTERVAL (V_NAME_F1 (cr_exp), 0x1p9, inf, 50000)
