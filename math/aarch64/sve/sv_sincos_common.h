/*
 * Core approximation for double-precision vector sincos
 *
 * Copyright (c) 2023-2026, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "sv_math.h"

static const struct sv_sincos_data
{
  double inv_pio4, pio4_1, pio4_2;
  double range_val;
  double s2, s4;
  double c2, c4;
  double s0, c0;
  double s1, s3, c1, c3;
  double sin_k_pi_over_4[8], cos_k_pi_over_4[8];
} sv_sincos_data = {
  .inv_pio4 = 0x1.45f306dc9c883p0,
  .pio4_1 = 0x1.921fb54442d18p-1,
  .pio4_2 = 0x1.1a62633145c06p-55,

  .s0 = -0x1.5555555555549p-3,
  .s1 = 0x1.111111110c9fcp-7,
  .s2 = -0x1.a01a017ee4324p-13,
  .s3 = 0x1.71ddce0d4d56ap-19,
  .s4 = -0x1.ad23d835c73a6p-26,

  .c0 = -0x1.fffffffffffafp-2,
  .c1 = 0x1.5555555546cd3p-5,
  .c2 = -0x1.6c16c135e6ff1p-10,
  .c3 = 0x1.a01951408961ep-16,
  .c4 = -0x1.26e05145290c1p-22,

  .range_val = 0x1p23,

  .sin_k_pi_over_4 = {
    0, 0x1.6a09e667f3bcdp-1,  1,	0x1.6a09e667f3bcdp-1,
    0, -0x1.6a09e667f3bcdp-1, -1, -0x1.6a09e667f3bcdp-1,
  },
  .cos_k_pi_over_4 = {
    1,  0x1.6a09e667f3bcdp-1,  0, -0x1.6a09e667f3bcdp-1,
    -1, -0x1.6a09e667f3bcdp-1, 0, 0x1.6a09e667f3bcdp-1,
  },
};

/* Double-precision vector function allowing calculation of both sin and cos in
   one function call, using shared argument reduction and addition angle trig
   identities. Only valid for inputs where |x| < 0x1p23.
   Worst-case error for sin is 2.67 + 0.5 ULP:
   sv_sincos_sin (0x1.022ae05e6dae2p+12)
    got 0x1.f7f7190cb05e9p-2
   want 0x1.f7f7190cb05e6p-2
   Worst-case error for cos is 2.68 + 0.5 ULP:
   sv_sincos_cos (0x1.d187aa5fac8f7p+3)
    got -0x1.98c7cd085e031p-2
   want -0x1.98c7cd085e02ep-2.  */
static inline svfloat64x2_t
sv_sincos_inline (svfloat64_t x, const struct sv_sincos_data *d)
{
  svbool_t ptrue = svptrue_b64 ();
  svfloat64_t invpio4_pio4_1 = svld1rq (ptrue, &d->inv_pio4);

  svfloat64_t n = svrinta_x (ptrue, svmul_lane (x, invpio4_pio4_1, 0));
  svuint64_t idx = svreinterpret_u64 (svcvt_s64_x (ptrue, n));

  svfloat64_t r = x;
  r = svmls_lane (r, n, invpio4_pio4_1, 1);
  r = svmls_x (svptrue_b64 (), r, n, sv_f64 (d->pio4_2));

  svfloat64_t r2 = svmul_x (ptrue, r, r);
  svfloat64_t r4 = svmul_x (ptrue, r2, r2);

  svfloat64_t s2s4 = svld1rq (ptrue, &d->s2);
  svfloat64_t c2c4 = svld1rq (ptrue, &d->c2);
  svfloat64_t s0c0 = svld1rq (ptrue, &d->s0);

  svfloat64_t sin_12 = svmla_lane (sv_f64 (d->s1), r2, s2s4, 0);
  svfloat64_t sin_34 = svmla_lane (sv_f64 (d->s3), r2, s2s4, 1);
  svfloat64_t sin_14 = svmla_x (ptrue, sin_12, r4, sin_34);

  svfloat64_t s0r2 = svmul_lane (r2, s0c0, 0);
  svfloat64_t sin_04 = svmla_x (ptrue, s0r2, r4, sin_14);
  svfloat64_t sin_r = svmla_x (ptrue, r, r, sin_04);

  svfloat64_t cos_12 = svmla_lane (sv_f64 (d->c1), r2, c2c4, 0);
  svfloat64_t cos_34 = svmla_lane (sv_f64 (d->c3), r2, c2c4, 1);
  svfloat64_t cos_14 = svmla_x (ptrue, cos_12, r4, cos_34);

  svfloat64_t r2c0 = svmul_lane (r2, s0c0, 1);
  svfloat64_t cosm1_r = svmla_x (ptrue, r2c0, r4, cos_14);

  /* Lookup sin(k * pi/4) and cos(k * pi/4).  */
  idx = svand_x (ptrue, idx, 7);
  svfloat64_t sin_k = svld1_gather_index (ptrue, d->sin_k_pi_over_4, idx);
  svfloat64_t cos_k = svld1_gather_index (ptrue, d->cos_k_pi_over_4, idx);

  /* Construct sin(x) and cos (x) from k and r, using angle addition formula,
    with approximations of sin(r) and cos(r) - 1 to reduce rounding errors.
    sin(x) = sin(k + r)
    = cos(k)*sin(r) + sin(k)*cos(r)
    = cos(k)*sin(r) + sin(k)*cosm1(r) + sin(k).
    cos(x) = cos(k + r)
      = cos(k)*cos(r) - sin(k)*sin(r)
      = cos(k)*cosm1(r) - sin(k)*sin(r) + cos(k).  */

  svfloat64_t sin = svmla_x (ptrue, sin_k, cos_k, sin_r);
  sin = svmla_x (ptrue, sin, sin_k, cosm1_r);

  svfloat64_t cos = svmla_x (ptrue, cos_k, cosm1_r, cos_k);
  cos = svmls_x (ptrue, cos, sin_k, sin_r);

  return svcreate2 (sin, cos);
}
