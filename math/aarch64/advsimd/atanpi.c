/*
 * Double-precision vector atanpi(x) function.
 *
 * Copyright (c) 2025, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "v_math.h"
#include "test_sig.h"
#include "test_defs.h"

static const struct data
{
  double c2, c4, c6, c8, c10, c12, c14, c16, c18, c20;
  float64x2_t c0, c1, c3, c5, c7, c9, c11, c13, c15, c17, c19;
} data = {
  /* Coefficients of polynomial P such that atanpi(x)~x*P(x^2) on
	      [2^-1022, 1.0].  */
  .c0 = V2 (0x1.45f306dc9c883p-2), .c1 = V2 (-0x1.b2995e7b7ba4ap-4),
  .c2 = 0x1.04c26be3d2c1p-4,	   .c3 = V2 (-0x1.7483759c17ea1p-5),
  .c4 = 0x1.21bb95c315d57p-5,	   .c5 = V2 (-0x1.da1bdc3d453f3p-6),
  .c6 = 0x1.912d20459b4bfp-6,	   .c7 = V2 (-0x1.5bbd4545cad1fp-6),
  .c8 = 0x1.331b83bec30a1p-6,	   .c9 = V2 (-0x1.13d6457f44de3p-6),
  .c10 = 0x1.f8e802974db94p-7,	   .c11 = V2 (-0x1.d7e173ab04a1ap-7),
  .c12 = 0x1.bdfa47d6a4f28p-7,	   .c13 = V2 (-0x1.9ba78f3232ceep-7),
  .c14 = 0x1.5e6044590ab4fp-7,	   .c15 = V2 (-0x1.01ccfdeb9f77fp-7),
  .c16 = 0x1.345cf0d4eb1c1p-8,	   .c17 = V2 (-0x1.19e5f00f67e3ap-9),
  .c18 = 0x1.6d3035ac7625bp-11,	   .c19 = V2 (-0x1.286bb9ae4ed79p-13),
  .c20 = 0x1.c37ec36da0e1ap-17,
};

#define SignMask v_u64 (0x8000000000000000)

/* Fast implementation of vector atanpi.
   atanpi(x) ~ shift + z * P(z^2) with reduction to [0,1] using
   z=1/x and shift = +-1/2. Maximum observed error is 2.76 ulps:
   _ZGVnN2v_atanpi(0x1.fa2d6912cd64fp-1) got 0x1.fc45a51bd497fp-3
					want 0x1.fc45a51bd497cp-3.  */
float64x2_t VPCS_ATTR V_NAME_D1 (atanpi) (float64x2_t x)
{
  const struct data *d = ptr_barrier (&data);

  uint64x2_t ix = vreinterpretq_u64_f64 (x);
  uint64x2_t sign = vandq_u64 (ix, SignMask);

  /* Argument Reduction:
     y := arctanpi(x) for |x| < 1
     y := arctanpi(-1/x) + 1/2 for x > 1
     y := arctanpi(-1/x) - 1/2 for x < -1
     Hence, use z=-1/a if |x|>=|-1|, otherwise z=a.  */
  uint64x2_t red = vcagtq_f64 (x, v_f64 (-1.0));
  float64x2_t z = vbslq_f64 (red, vdivq_f64 (v_f64 (-1.0), x), x);

  /* Shift is calculated as +1/2 or 0, depending on the argument case.  */
  float64x2_t shift = vreinterpretq_f64_u64 (
      vandq_u64 (red, vreinterpretq_u64_f64 (v_f64 (0.5))));

  /* Reinsert sign bit from argument into the shift value.  */
  shift = vreinterpretq_f64_u64 (
      veorq_u64 (vreinterpretq_u64_f64 (shift), sign));

  /* Calculate polynomial approximation P(z^2) with deg(P)=19.  */
  float64x2_t z2 = vmulq_f64 (z, z);
  float64x2_t z4 = vmulq_f64 (z2, z2);
  float64x2_t z8 = vmulq_f64 (z4, z4);
  float64x2_t z16 = vmulq_f64 (z8, z8);

  float64x2_t c24 = vld1q_f64 (&d->c2);
  float64x2_t c68 = vld1q_f64 (&d->c6);

  /* Order-7 Estrin.  */
  float64x2_t p12 = vfmaq_laneq_f64 (d->c1, z2, c24, 0);
  float64x2_t p34 = vfmaq_laneq_f64 (d->c3, z2, c24, 1);
  float64x2_t p56 = vfmaq_laneq_f64 (d->c5, z2, c68, 0);
  float64x2_t p78 = vfmaq_laneq_f64 (d->c7, z2, c68, 1);

  float64x2_t p14 = vfmaq_f64 (p12, z4, p34);
  float64x2_t p58 = vfmaq_f64 (p56, z4, p78);
  float64x2_t p18 = vfmaq_f64 (p14, z8, p58);

  /* Order-11 Estrin.  */
  float64x2_t c1012 = vld1q_f64 (&d->c10);
  float64x2_t c1416 = vld1q_f64 (&d->c14);
  float64x2_t c1820 = vld1q_f64 (&d->c18);

  float64x2_t p910 = vfmaq_laneq_f64 (d->c9, z2, c1012, 0);
  float64x2_t p1112 = vfmaq_laneq_f64 (d->c11, z2, c1012, 1);
  float64x2_t p912 = vfmaq_f64 (p910, z4, p1112);

  float64x2_t p1314 = vfmaq_laneq_f64 (d->c13, z2, c1416, 0);
  float64x2_t p1516 = vfmaq_laneq_f64 (d->c15, z2, c1416, 1);
  float64x2_t p1316 = vfmaq_f64 (p1314, z4, p1516);

  float64x2_t p1718 = vfmaq_laneq_f64 (d->c17, z2, c1820, 0);
  float64x2_t p1920 = vfmaq_laneq_f64 (d->c19, z2, c1820, 1);
  float64x2_t p1720 = vfmaq_f64 (p1718, z4, p1920);

  float64x2_t p916 = vfmaq_f64 (p912, z8, p1316);
  float64x2_t p920 = vfmaq_f64 (p916, z16, p1720);

  float64x2_t y = vfmaq_f64 (p18, p920, z16);

  y = vfmaq_f64 (d->c0, z2, y);

  /* y = shift + z * p(z^2).  */
  return vfmaq_f64 (shift, z, y);
}

#if WANT_C23_TESTS
TEST_ULP (V_NAME_D1 (atanpi), 2.26)
TEST_DISABLE_FENV (V_NAME_D1 (atanpi))
TEST_INTERVAL (V_NAME_D1 (atanpi), 0, 0x1p-30, 10000)
TEST_INTERVAL (V_NAME_D1 (atanpi), -0, -0x1p-30, 1000)
TEST_INTERVAL (V_NAME_D1 (atanpi), 0x1p-30, 0x1p53, 900000)
TEST_INTERVAL (V_NAME_D1 (atanpi), -0x1p-30, -0x1p53, 90000)
TEST_INTERVAL (V_NAME_D1 (atanpi), 0x1p53, inf, 10000)
TEST_INTERVAL (V_NAME_D1 (atanpi), -0x1p53, -inf, 1000)
#endif
