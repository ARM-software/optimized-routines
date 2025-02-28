/*
 * Double-precision SVE asin(x) function.
 *
 * Copyright (c) 2025, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "sv_math.h"
#include "test_defs.h"

static const struct data
{
  float64_t c1, c3, c5, c7, c9, c11;
  float64_t c0, c2, c4, c6, c8, c10;
  float64_t pi_over_2, inv_pi;
} data = {
  /* Polynomial approximation of  (asin(sqrt(x)) - sqrt(x)) / (x * sqrt(x))
     on [ 0x1p-106, 0x1p-2 ], relative error: 0x1.c3d8e169p-57.  */
  .c0 = 0x1.555555555554ep-3,	     .c1 = 0x1.3333333337233p-4,
  .c2 = 0x1.6db6db67f6d9fp-5,	     .c3 = 0x1.f1c71fbd29fbbp-6,
  .c4 = 0x1.6e8b264d467d6p-6,	     .c5 = 0x1.1c5997c357e9dp-6,
  .c6 = 0x1.c86a22cd9389dp-7,	     .c7 = 0x1.856073c22ebbep-7,
  .c8 = 0x1.fd1151acb6bedp-8,	     .c9 = 0x1.087182f799c1dp-6,
  .c10 = -0x1.6602748120927p-7,	     .c11 = 0x1.cfa0dd1f9478p-6,
  .pi_over_2 = 0x1.921fb54442d18p+0, .inv_pi = 0x1.45f306dc9c883p-2,
};

/* Double-precision SVE implementation of vector asinpi(x).

   For |x| in [0, 0.5], use an order 11 polynomial P such that the final
   approximation is an odd polynomial: asin(x) ~ x + x^3 P(x^2).

   The largest observed error in this region is 1.32 ulp:
   _ZGVsMxv_asinpi (0x1.fc12356dbdefbp-2) got 0x1.5272e9658ba66p-3
					 want 0x1.5272e9658ba64p-3

   For |x| in [0.5, 1.0], use same approximation with a change of variable:
  asin(x) = pi/2 - (y + y * z * P(z)), with  z = (1-x)/2 and y = sqrt(z).

   The largest observed error in this region is 3.48 ulp:
   _ZGVsMxv_asinpi (0x1.03da0c2295424p-1) got 0x1.5b02b3dcafaefp-3
					 want 0x1.5b02b3dcafaf2p-3.  */
svfloat64_t SV_NAME_D1 (asinpi) (svfloat64_t x, const svbool_t pg)
{
  const struct data *d = ptr_barrier (&data);
  svbool_t ptrue = svptrue_b64 ();

  svuint64_t sign = svand_x (pg, svreinterpret_u64 (x), 0x8000000000000000);
  svfloat64_t ax = svabs_x (pg, x);
  svbool_t a_ge_half = svacge (pg, x, 0.5);

  /* Evaluate polynomial Q(x) = y + y * z * P(z) with
     z = x ^ 2 and y = |x|            , if |x| < 0.5
     z = (1 - |x|) / 2 and y = sqrt(z), if |x| >= 0.5.  */
  svfloat64_t z2 = svsel (a_ge_half, svmls_x (pg, sv_f64 (0.5), ax, 0.5),
			  svmul_x (ptrue, x, x));
  svfloat64_t z = svsqrt_m (ax, a_ge_half, z2);

  /* Use a single polynomial approximation P for both intervals.  */
  svfloat64_t z3 = svmul_x (pg, z2, z);
  svfloat64_t z4 = svmul_x (pg, z2, z2);
  svfloat64_t z8 = svmul_x (pg, z4, z4);

  svfloat64_t c13 = svld1rq (ptrue, &d->c1);
  svfloat64_t c57 = svld1rq (ptrue, &d->c5);
  svfloat64_t c911 = svld1rq (ptrue, &d->c9);

  /* Order-11 Estrin scheme.  */
  svfloat64_t p01 = svmla_lane (sv_f64 (d->c0), z2, c13, 0);
  svfloat64_t p23 = svmla_lane (sv_f64 (d->c2), z2, c13, 1);
  svfloat64_t p03 = svmla_x (pg, p01, z4, p23);

  svfloat64_t p45 = svmla_lane (sv_f64 (d->c4), z2, c57, 0);
  svfloat64_t p67 = svmla_lane (sv_f64 (d->c6), z2, c57, 1);
  svfloat64_t p47 = svmla_x (pg, p45, z4, p67);

  svfloat64_t p89 = svmla_lane (sv_f64 (d->c8), z2, c911, 0);
  svfloat64_t p1011 = svmla_lane (sv_f64 (d->c10), z2, c911, 1);
  svfloat64_t p811 = svmla_x (pg, p89, z4, p1011);

  svfloat64_t p411 = svmla_x (pg, p47, z8, p811);
  svfloat64_t p = svmla_x (pg, p03, z8, p411);

  /* Finalize polynomial: z + z3 * P(z2).  */
  p = svmla_x (pg, z, z3, p);

  /* asin(|x|) = Q(|x|)         , for |x| < 0.5
	       = pi/2 - 2 Q(|x|), for |x| >= 0.5.  */
  svfloat64_t y = svmad_m (a_ge_half, p, sv_f64 (-2.0), d->pi_over_2);

  /* Reinsert the sign from the argument.  */
  svfloat64_t inv_pi = svreinterpret_f64 (
      svorr_x (pg, svreinterpret_u64 (sv_f64 (d->inv_pi)), sign));

  return svmul_x (pg, y, inv_pi);
}

#if WANT_TRIGPI_TESTS
TEST_ULP (SV_NAME_D1 (asinpi), 2.98)
TEST_DISABLE_FENV (SV_NAME_D1 (asinpi))
TEST_INTERVAL (SV_NAME_D1 (asinpi), 0, 0.5, 50000)
TEST_INTERVAL (SV_NAME_D1 (asinpi), 0.5, 1.0, 50000)
TEST_INTERVAL (SV_NAME_D1 (asinpi), 1.0, 0x1p11, 50000)
TEST_INTERVAL (SV_NAME_D1 (asinpi), 0x1p11, inf, 20000)
TEST_INTERVAL (SV_NAME_D1 (asinpi), -0, -inf, 20000)
#endif
CLOSE_SVE_ATTR
