/*
 * Single-precision SVE hypot(x) function.
 *
 * Copyright (c) 2023-2026, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "sv_math.h"
#include "test_sig.h"
#include "test_defs.h"

#define TinyBound 0x0c800000 /* asuint (0x1p-102).  */
#define Thres 0x73000000     /* 0x70000000 - TinyBound.  */

static const struct data
{
  uint32_t tiny_bound;
  uint32_t thres;
  float32_t scale;
  float32_t inv_scale;
  float32_t scale_up_thres;
  float32_t scale_down_thres;
  float32_t inf;
} data = {
  .tiny_bound = TinyBound,
  .thres = Thres,
  /* 75 is the minimal symmetric exponent such that all single-precision
    inputs, including subnormals, can be handled safely. The smallest positive
    float is 2^-149. When scaling inputs by 2^75 before squaring, we get:
      (2^-149 · 2^75)^2 = 2^-148
    which is representable as a subnormal float.

    Therefore, scale = 2^75 (and invScale = 2^-75) is the smallest power-of-two
    scaling that prevents underflow in x*x + y*y while remaining symmetric for
    scaling back the final result.  */
  .scale = 0x1p-75f,
  .inv_scale = 0x1p+75f,
  .scale_up_thres = 0x1p+60f,
  .scale_down_thres = 0x1p-60f,
  .inf = INFINITY,
};

static svfloat32_t NOINLINE
special_case (svfloat32_t sqsum, svfloat32_t x, svfloat32_t y, svbool_t pg,
	      svbool_t special, const struct data *d)
{
  svfloat32_t ax = svabs_x (special, x);
  svfloat32_t ay = svabs_x (special, y);
  svfloat32_t amin = svmin_x (special, ax, ay);
  svfloat32_t amax = svmax_x (special, ax, ay);
  svbool_t mask_big = svcmpgt (special, amax, sv_f32 (d->scale_up_thres));
  svbool_t mask_tiny = svcmplt (special, amin, sv_f32 (d->scale_down_thres));

  /* Implemented vectorised fallback for AdvSIMD hypotf special case
    for large and tiny values by avoiding underflow/overflow caused
    by squaring the inputs.  */
  svfloat32_t scale = sv_f32 (1.0f);
  svfloat32_t inv_scale = sv_f32 (1.0f);
  scale = svsel (mask_tiny, sv_f32 (d->inv_scale), scale);
  scale = svsel (mask_big, sv_f32 (d->scale), scale);
  inv_scale = svsel (mask_tiny, sv_f32 (d->scale), inv_scale);
  inv_scale = svsel (mask_big, sv_f32 (d->inv_scale), inv_scale);
  x = svmul_x (special, amax, scale);
  y = svmul_x (special, amin, scale);

  /* The following lines are written to choose the algebric formulation that
    gives the smaller rounding error for the current magnitude ratio. They
    represent a^2 + b^2, not the hypot itself:
    - sum1 = (a-b)^2 + (2*b)*a where t1=2*b and t2=a-b
    - sum2 which is just a^2 + b^2.  */
  svfloat32_t t1 = svadd_x (special, y, y);
  svfloat32_t t2 = svsub_x (special, x, y);
  svfloat32_t sum1 = svmla_x (special, svmul_x (special, t2, t2), t1, x);
  svfloat32_t sum2 = svmla_x (special, svmul_x (special, y, y), x, x);
  svbool_t mask_fma = svcmpge (special, t1, x);
  svfloat32_t res = svmul_x (
      special, svsqrt_x (special, svsel (mask_fma, sum1, sum2)), inv_scale);

  /* Handles cases where either x or y are not finite, returning infinity when
    needed. Svmin & svmax will return NaN whenever one parameter is NaN. We
    add an extra 'inf_mask' to make sure infinity is returned when either
    absolute x or absolute y are infinity and the other value is NaN.

    This also handles cases of x = inf and y = inf, which will become NaN when
    subtracted above, using svsub.  */
  svbool_t inf_mask
      = svorr_b_z (special, svcmpeq (special, ax, sv_f32 (d->inf)),
		   svcmpeq (special, ay, sv_f32 (d->inf)));
  res = svsel (inf_mask, sv_f32 (d->inf), res);
  return svsel (special, res, svsqrt_x (pg, sqsum));
}

/* SVE implementation of single-precision hypot.
   Maximum error observed is 1.21 ULP:
   _ZGVsMxvv_hypotf (0x1.6a213cp-19, -0x1.32b982p-26) got 0x1.6a2346p-19
						     want 0x1.6a2344p-19.  */
svfloat32_t SV_NAME_F2 (hypot) (svfloat32_t x, svfloat32_t y,
				const svbool_t pg)
{
  const struct data *d = ptr_barrier (&data);
  svfloat32_t sqsum = svmla_x (pg, svmul_x (pg, x, x), y, y);

  svbool_t special = svcmpge (
      pg, svsub_x (pg, svreinterpret_u32 (sqsum), TinyBound), Thres);

  if (unlikely (svptest_any (pg, special)))
    return special_case (sqsum, x, y, pg, special, d);

  return svsqrt_x (pg, sqsum);
}

TEST_SIG (SV, F, 2, hypot, -10.0, 10.0)
TEST_ULP (SV_NAME_F2 (hypot), 0.71)
TEST_INTERVAL2 (SV_NAME_F2 (hypot), 0, inf, 0, inf, 10000)
TEST_INTERVAL2 (SV_NAME_F2 (hypot), 0, inf, -0, -inf, 10000)
TEST_INTERVAL2 (SV_NAME_F2 (hypot), -0, -inf, 0, inf, 10000)
TEST_INTERVAL2 (SV_NAME_F2 (hypot), -0, -inf, -0, -inf, 10000)
CLOSE_SVE_ATTR
