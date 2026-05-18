/*
 * Core approximation for double-precision vector sincos
 *
 * Copyright (c) 2023-2026, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "v_math.h"
#include "v_poly_f64.h"
#include "v_trig_fallback.h"

static const struct v_sincos_data
{
  double inv_pio4, pio4_1, pio4_2, pio4_3;
  float64x2_t range_val;
  double s2, s4;
  double c2, c4;
  double s0, c0;
  float64x2_t s1, s3, c1, c3;
  double sincos_k_pi_over_4[8][2];
} v_sincos_data = {
  .inv_pio4 = 0x1.45f306dc9c883p0,
  .pio4_1 = 0x1.921fb54442d18p-1,
  .pio4_2 = 0x1.1a62633145c06p-55,
  .pio4_3 = 0x1.c1cd129024e09p-108,

  .s0 = -0x1.5555555555549p-3,
  .s1 = V2 (0x1.111111110c9fcp-7),
  .s2 = -0x1.a01a017ee4324p-13,
  .s3 = V2 (0x1.71ddce0d4d56ap-19),
  .s4 = -0x1.ad23d835c73a6p-26,

  .c0 = -0x1.fffffffffffafp-2,
  .c1 = V2 (0x1.5555555546cd3p-5),
  .c2 = -0x1.6c16c135e6ff1p-10,
  .c3 = V2 (0x1.a01951408961ep-16),
  .c4 = -0x1.26e05145290c1p-22,

  .range_val = V2 (0x1p23),

  .sincos_k_pi_over_4 = {
    { 0, 1 },
    { 0x1.6a09e667f3bcdp-1, 0x1.6a09e667f3bcdp-1 },
    { 1, 0 },
    { 0x1.6a09e667f3bcdp-1, -0x1.6a09e667f3bcdp-1 },
    { 0, -1 },
    { -0x1.6a09e667f3bcdp-1, -0x1.6a09e667f3bcdp-1 },
    { -1, 0 },
    { -0x1.6a09e667f3bcdp-1, 0x1.6a09e667f3bcdp-1 },
  },
};

/* Double-precision vector function allowing calculation of both sin and cos in
   one function call, using shared argument reduction and addition angle trig
   identities. Only valid for inputs where |x| < 0x1p23.
   Worst-case error for sin is 2.67 + 0.5 ULP:
   v_sincos_sin (0x1.022ae05e6dae2p+12)
    got 0x1.f7f7190cb05e9p-2
   want 0x1.f7f7190cb05e6p-2
   Worst-case error for cos is 2.68 + 0.5 ULP:
   v_sincos_cos (0x1.d187aa5fac8f7p+3)
    got -0x1.98c7cd085e031p-2
   want -0x1.98c7cd085e02ep-2.  */
static inline float64x2x2_t VPCS_ATTR
v_sincos_inline (float64x2_t x, const struct v_sincos_data *d)
{
  float64x2_t invpio4_pio4_1 = vld1q_f64 (&d->inv_pio4);
  float64x2_t pio4_2_3 = vld1q_f64 (&d->pio4_2);

  float64x2_t n = vrndaq_f64 (vmulq_laneq_f64 (x, invpio4_pio4_1, 0));
  uint64x2_t idx = vreinterpretq_u64_s64 (vcvtq_s64_f64 (n));

  float64x2_t r = x;
  r = vfmsq_laneq_f64 (r, n, invpio4_pio4_1, 1);
  r = vfmsq_laneq_f64 (r, n, pio4_2_3, 0);
  r = vfmsq_laneq_f64 (r, n, pio4_2_3, 1);

  float64x2_t r2 = vmulq_f64 (r, r);
  float64x2_t r4 = vmulq_f64 (r2, r2);

  float64x2_t s2s4 = vld1q_f64 (&d->s2);
  float64x2_t c2c4 = vld1q_f64 (&d->c2);
  float64x2_t s0c0 = vld1q_f64 (&d->s0);

  float64x2_t sin_12 = vfmaq_laneq_f64 (d->s1, r2, s2s4, 0);
  float64x2_t sin_34 = vfmaq_laneq_f64 (d->s3, r2, s2s4, 1);
  float64x2_t sin_14 = vfmaq_f64 (sin_12, r4, sin_34);

  float64x2_t s0r2 = vmulq_laneq_f64 (r2, s0c0, 0);
  float64x2_t sin_04 = vfmaq_f64 (s0r2, r4, sin_14);
  float64x2_t sin_r = vfmaq_f64 (r, r, sin_04);

  float64x2_t cos_12 = vfmaq_laneq_f64 (d->c1, r2, c2c4, 0);
  float64x2_t cos_34 = vfmaq_laneq_f64 (d->c3, r2, c2c4, 1);
  float64x2_t cos_14 = vfmaq_f64 (cos_12, r4, cos_34);

  float64x2_t c0r2 = vmulq_laneq_f64 (r2, s0c0, 1);
  float64x2_t cosm1_r = vfmaq_f64 (c0r2, r4, cos_14);

  unsigned idx0 = vgetq_lane_u64 (idx, 0) & 7;
  unsigned idx1 = vgetq_lane_u64 (idx, 1) & 7;

  float64x2_t sin_cos_k0 = vld1q_f64 (&d->sincos_k_pi_over_4[idx0][0]);
  float64x2_t sin_cos_k1 = vld1q_f64 (&d->sincos_k_pi_over_4[idx1][0]);

  float64x2_t sin_k = vzip1q_f64 (sin_cos_k0, sin_cos_k1);
  float64x2_t cos_k = vzip2q_f64 (sin_cos_k0, sin_cos_k1);

  /* Construct sin(x) and cos (x) from k and r, using angle addition formula,
  with approximations of sin(r) and cos(r) - 1 to reduce rounding errors.
  sin(x) = sin(k + r)
  = cos(k)*sin(r) + sin(k)*cos(r)
  = cos(k)*sin(r) + sin(k)*cosm1(r) + sin(k).
  cos(x) = cos(k + r)
    = cos(k)*cos(r) - sin(k)*sin(r)
    = cos(k)*cosm1(r) - sin(k)*sin(r) + cos(k).  */

  float64x2_t sin = vfmaq_f64 (sin_k, cos_k, sin_r);
  sin = vfmaq_f64 (sin, sin_k, cosm1_r);

  float64x2_t cos = vfmaq_f64 (cos_k, cosm1_r, cos_k);
  cos = vfmsq_f64 (cos, sin_k, sin_r);

  return (float64x2x2_t){ sin, cos };
}

/* Double-precision vector function allowing calculation of both sin and cos
   for large x in one function call, using shared argument reduction and
   addition angle trig identities.
   Worst-case error for sin is 2.15 + 0.5 ULP when |x| >= 0x1p23:
   v_sincos_sin (0x1.3d4ded894041ep+784)
    got -0x1.fffa6b28930b5p-7
   want -0x1.fffa6b28930b2p-7
   Worst-case error for cos is 2.44 + 0.5 ULP when |x| >= 0x1p23:
   v_sincos_cos (0x1.aac6f8bffec82p+206)
    got -0x1.98ecd0b3020bfp-7
   want -0x1.98ecd0b3020bcp-7.  */
static inline float64x2x2_t
v_sincos_fallback (float64x2_t x)
{
  const struct reduction_data *d = ptr_barrier (&reduction_data);
  struct reduction_result_t r = v_large_range_reduction (x, d);

  float64x2x2_t eval = v_sincos_eval (r.remainder, d);
  float64x2x2_t lookup = v_sincos_lookup (r.quadrant);

  float64x2_t sin_r = eval.val[0];
  float64x2_t cosm1_r = eval.val[1];
  float64x2_t sin_k = lookup.val[0];
  float64x2_t cos_k = lookup.val[1];

  /* Construct sin(x) and cos (x) from k and r, using angle addition formula,
    with approximations of sin(r) and cos(r) - 1 to reduce rounding errors.
    sin(x) = sin(k + r)
    = cos(k)*sin(r) + sin(k)*cos(r)
    = cos(k)*sin(r) + sin(k)*cosm1(r) + sin(k).
    cos(x) = cos(k + r)
      = cos(k)*cos(r) - sin(k)*sin(r)
      = cos(k)*cosm1(r) - sin(k)*sin(r) + cos(k).  */

  float64x2_t sin = vfmaq_f64 (sin_k, cos_k, sin_r);
  sin = vfmaq_f64 (sin, sin_k, cosm1_r);

  float64x2_t cos = vfmaq_f64 (cos_k, cosm1_r, cos_k);
  cos = vfmsq_f64 (cos, sin_k, sin_r);

  float64x2x2_t result;
  result.val[0] = sin;
  result.val[1] = cos;
  return result;
}
