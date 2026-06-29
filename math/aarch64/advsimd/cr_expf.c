/*
 * Single-precision correctly rounded vector e^x function.
 *
 * Copyright (c) 2026, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "v_math.h"
#include "test_defs.h"
#include "test_sig.h"

static const struct data
{
  float64x2_t shift, inv_ln2;
  double ln2_hi, ln2_lo;
  double c1, c3;
  float64x2_t c0, c2;
  float32x4_t range_val;
  uint32x4_t inf;
  uint64x2_t idx_mask;
  uint64_t mantissa_table[64];
} expf_data = {
  .shift = V2 (0x1.800000000ffc0p+46),
  .inv_ln2 = V2 (0x1.71547652b82fep+0),
  .ln2_hi = 0x1.62e42fefa39efp-1,
  .ln2_lo = 0x1.abc9e3b39803fp-56,
  .c0 = V2 (0x1.fffffffffdbcep-2),
  .c1 = 0x1.55555555543c2p-3,
  .c2 = V2 (0x1.555573c64f2e3p-5),
  .c3 = 0x1.111126b4eff73p-7,
  .idx_mask = V2 (0x3f),
  .range_val = V4 (0x1p+9),
  .inf = V4 (0x7f800000),
  .mantissa_table = {
    0x0000000000000, 0x02c9a3e778061, 0x059b0d3158574, 0x0874518759bc8,
    0x0b5586cf9890f, 0x0e3ec32d3d1a2, 0x11301d0125b51, 0x1429aaea92de0,
    0x172b83c7d517b, 0x1a35beb6fcb75, 0x1d4873168b9aa, 0x2063b88628cd6,
    0x2387a6e756238, 0x26b4565e27cdd, 0x29e9df51fdee1, 0x2d285a6e4030b,
    0x306fe0a31b715, 0x33c08b26416ff, 0x371a7373aa9cb, 0x3a7db34e59ff7,
    0x3dea64c123422, 0x4160a21f72e2a, 0x44e086061892d, 0x486a2b5c13cd0,
    0x4bfdad5362a27, 0x4f9b2769d2ca7, 0x5342b569d4f82, 0x56f4736b527da,
    0x5ab07dd485429, 0x5e76f15ad2148, 0x6247eb03a5585, 0x6623882552225,
    0x6a09e667f3bcd, 0x6dfb23c651a2f, 0x71f75e8ec5f74, 0x75feb564267c9,
    0x7a11473eb0187, 0x7e2f336cf4e62, 0x82589994cce13, 0x868d99b4492ed,
    0x8ace5422aa0db, 0x8f1ae99157736, 0x93737b0cdc5e5, 0x97d829fde4e50,
    0x9c49182a3f090, 0xa0c667b5de565, 0xa5503b23e255d, 0xa9e6b5579fdbf,
    0xae89f995ad3ad, 0xb33a2b84f15fb, 0xb7f76f2fb5e47, 0xbcc1e904bc1d2,
    0xc199bdd85529c, 0xc67f12e57d14b, 0xcb720dcef9069, 0xd072d4a07897c,
    0xd5818dcfba487, 0xda9e603db3285, 0xdfc97337b9b5f, 0xe502ee78b3ff6,
    0xea4afa2a490da, 0xefa1bee615a27, 0xf50765b6e4540, 0xfa7c1819e90d8,
  },
};

static inline float64x2_t VPCS_ATTR
exp_lookup (uint64x2_t u, const struct data *data)
{
  uint64_t idx0 = vgetq_lane_u64 (u & data->idx_mask, 0);
  uint64_t idx1 = vgetq_lane_u64 (u & data->idx_mask, 1);

  uint64_t mant0 = data->mantissa_table[idx0];
  uint64_t mant1 = data->mantissa_table[idx1];

  uint64x2_t mantissa = vdupq_n_u64 (mant0);
  mantissa = vsetq_lane_u64 (mant1, mantissa, 1);

  uint64x2_t mask = vdupq_n_u64 (0xfff0000000000000);
  uint64x2_t exponent = vshlq_n_u64 (u, 46);
  uint64x2_t result = vbslq_u64 (mask, exponent, mantissa);

  return vreinterpretq_f64_u64 (result);
}

static inline float64x2_t VPCS_ATTR
inline_exp (float64x2_t x, const struct data *data)
{

  float64x2_t z = vfmaq_f64 (data->shift, x, data->inv_ln2);
  float64x2_t n = vsubq_f64 (z, data->shift);

  float64x2_t ln2 = vld1q_f64 (&data->ln2_hi);

  float64x2_t r = x;
  r = vfmsq_laneq_f64 (r, n, ln2, 0);
  r = vfmsq_laneq_f64 (r, n, ln2, 1);

  float64x2_t coeffs = vld1q_f64 (&data->c1);

  /* poly(r) = exp(r) - 1 ~= r + c0*r^2 + c1*r^3 + c2*r^4 + c3*r^5.  */
  float64x2_t r2 = r * r;
  float64x2_t p01 = vfmaq_laneq_f64 (data->c0, r, coeffs, 0);
  float64x2_t p23 = vfmaq_laneq_f64 (data->c2, r, coeffs, 1);
  float64x2_t p04 = vfmaq_f64 (p01, r2, p23);
  float64x2_t y = vfmaq_f64 (r, r2, p04);

  uint64x2_t u = vreinterpretq_u64_f64 (z);
  float64x2_t s = exp_lookup (u, data);

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
