/*
 * Double-precision SVE hypot(x) function.
 *
 * Copyright (c) 2023-2026, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "sv_math.h"
#include "test_sig.h"
#include "test_defs.h"

static const struct data
{
  uint64_t tiny_bound, thres;
  float64_t scale, inv_scale;
  float64_t scale_up_thres;
  float64_t scale_down_thres;
  float64_t inf;
} data = { .tiny_bound = 0x0c80000000000000, /* asuint (0x1p-102).  */
	   .thres = 0x7370000000000000,	     /* asuint (inf) - tiny_bound.  */
	   /* 537 is the minimal symmetric exponent such that all
		   double-precision inputs, including subnormals, can be
       handled safely. The smallest positive double is 2^-1074. When scaling
		   inputs by 2^537 before squaring, we get: (2^-1074 · 2^537)^2
       = 2^-537 which is representable as a subnormal double.

		   Therefore, scale = 2^537 (and invScale = 2^-537) is the
	      smallest power-of-two scaling that prevents underflow in x*x +
       y*y while remaining symmetric for scaling back the final result.  */
	   .scale = 0x1p-537,
	   .inv_scale = 0x1p+537,
	   .scale_up_thres = 0x1p+511,
	   .scale_down_thres = 0x1p-459,
	   .inf = INFINITY };

static svfloat64_t NOINLINE
special_case (svfloat64_t sqsum, svfloat64_t x, svfloat64_t y, svbool_t pg,
	      svbool_t special, const struct data *d)
{
  svfloat64_t ax = svabs_x (special, x);
  svfloat64_t ay = svabs_x (special, y);
  svfloat64_t amin = svmin_x (special, ax, ay);
  svfloat64_t amax = svmax_x (special, ax, ay);
  svbool_t mask_big = svcmpgt (special, amax, sv_f64 (d->scale_up_thres));
  svbool_t mask_tiny = svcmplt (special, amin, sv_f64 (d->scale_down_thres));

  /* Implemented vectorised fallback for SVE hypotf special case
    for large and tiny values by avoiding underflow/overflow caused
    by squaring the inputs.  */
  svfloat64_t scale = sv_f64 (1.0f);
  svfloat64_t inv_scale = sv_f64 (1.0f);
  scale = svsel (mask_tiny, sv_f64 (d->inv_scale), scale);
  scale = svsel (mask_big, sv_f64 (d->scale), scale);
  inv_scale = svsel (mask_tiny, sv_f64 (d->scale), inv_scale);
  inv_scale = svsel (mask_big, sv_f64 (d->inv_scale), inv_scale);
  x = svmul_x (special, amax, scale);
  y = svmul_x (special, amin, scale);

  /* The following lines are written to choose the algebric formulation that
    gives the smaller rounding error for the current magnitude ratio. They
    represent a^2 + b^2, not the hypot itself:
    - sum1 = (a-b)^2 + (2*b)*a where t1=2*b and t2=a-b
    - sum2 which is just a^2 + b^2.  */
  svfloat64_t t1 = svadd_x (special, y, y);
  svfloat64_t t2 = svsub_x (special, x, y);
  svfloat64_t sum1 = svmla_x (special, svmul_x (special, t2, t2), t1, x);
  svfloat64_t sum2 = svmla_x (special, svmul_x (special, y, y), x, x);
  svbool_t mask_fma = svcmpge (special, t1, x);
  svfloat64_t res = svmul_x (
      special, svsqrt_x (special, svsel (mask_fma, sum1, sum2)), inv_scale);

  /* Handles cases where either x or y are not finite, returning infinity when
    needed. Svmin & svmax will return NaN whenever one parameter is NaN. We
    add an extra 'inf_mask' to make sure infinity is returned when either
    absolute x or absolute y are infinity and the other value is NaN.

    This also handles cases of x = inf and y = inf, which will become NaN when
    subtracted above, using svsub.  */
  svbool_t inf_mask
      = svorr_b_z (special, svcmpeq (special, ax, sv_f64 (d->inf)),
		   svcmpeq (special, ay, sv_f64 (d->inf)));
  res = svsel (inf_mask, sv_f64 (d->inf), res);
  return svsel (special, res, svsqrt_x (pg, sqsum));
}

/* SVE implementation of double-precision hypot.
   Maximum error observed is 1.21 ULP:
   _ZGVsMxvv_hypot (-0x1.6a22d0412cdd3p+352, 0x1.d3d89bd66fb1ap+330)
    got 0x1.6a22d0412cfp+352
   want 0x1.6a22d0412cf01p+352.  */
svfloat64_t SV_NAME_D2 (hypot) (svfloat64_t x, svfloat64_t y, svbool_t pg)
{
  const struct data *d = ptr_barrier (&data);

  svfloat64_t sqsum = svmla_x (pg, svmul_x (pg, x, x), y, y);

  svbool_t special = svcmpge (
      pg, svsub_x (pg, svreinterpret_u64 (sqsum), d->tiny_bound), d->thres);

  if (unlikely (svptest_any (pg, special)))
    return special_case (sqsum, x, y, pg, special, d);
  return svsqrt_x (pg, sqsum);
}

TEST_SIG (SV, D, 2, hypot, -10.0, 10.0)
TEST_ULP (SV_NAME_D2 (hypot), 0.71)
TEST_INTERVAL2 (SV_NAME_D2 (hypot), 0, inf, 0, inf, 10000)
TEST_INTERVAL2 (SV_NAME_D2 (hypot), 0, inf, -0, -inf, 10000)
TEST_INTERVAL2 (SV_NAME_D2 (hypot), -0, -inf, 0, inf, 10000)
TEST_INTERVAL2 (SV_NAME_D2 (hypot), -0, -inf, -0, -inf, 10000)
CLOSE_SVE_ATTR
