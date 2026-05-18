/*
 * Core approximation for single-precision vector sincos
 *
 * Copyright (c) 2023-2026, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "sv_math.h"
#include "sv_trigf_fallback.h"

static const struct trig_data
{
  float neg_pio2_1, neg_pio2_2, neg_pio2_3, inv_pio2, shift, range_val;
  float sin_start, cos_start;
} trig_data = {
  /* Polynomial coefficients are hard-wired in FTMAD instructions.  */
  .neg_pio2_1 = -0x1.921fb6p+0f,
  .neg_pio2_2 = 0x1.777a5cp-25f,
  .neg_pio2_3 = 0x1.ee59dap-50f,
  .inv_pio2 = 0x1.45f306p-1f,
  /* Original shift used in AdvSIMD cosf,
     plus a contribution to set the bit #0 of q
     as expected by trigonometric instructions.  */
  .shift = 0x1.8p+23f,
  .range_val = 0x1p20f,
  /* Values used by svtmad when #4 is passed in,
     Loaded directly for better performance.  */
  .sin_start = 0x1.6d3adap-19f,
  .cos_start = 0x1.9a6f98p-16f,
};

static inline svfloat32x2_t
sv_sincosf_inline (svfloat32_t x, const struct trig_data *d)
{
  svbool_t ptrue = svptrue_b32 ();

  /* Load some constants in quad-word chunks to minimise memory access.  */
  svfloat32_t negpio2_and_invpio2 = svld1rq (ptrue, &d->neg_pio2_1);

  /* n = rint(x/(pi/2)).  */
  svfloat32_t q = svmla_lane (sv_f32 (d->shift), x, negpio2_and_invpio2, 3);
  svfloat32_t n = svsub_x (ptrue, q, d->shift);

  /* r = x - n*(pi/2)  (range reduction into -pi/4 .. pi/4).  */
  svfloat32_t r = x;
  r = svmla_lane (r, n, negpio2_and_invpio2, 0);
  r = svmla_lane (r, n, negpio2_and_invpio2, 1);
  r = svmla_lane (r, n, negpio2_and_invpio2, 2);

  svuint32_t sin_q = svreinterpret_u32 (q);
  svuint32_t cos_q = svadd_x (ptrue, svreinterpret_u32 (q), 1);

  svfloat32_t sin_f = svtssel (r, sin_q);
  svfloat32_t sin_r2 = svtsmul (r, sin_q);

  svfloat32_t cos_f = svtssel (r, cos_q);
  svfloat32_t cos_r2 = svtsmul (r, cos_q);

  /* Manually selecting the starting value saves a redundant svtmad.  */
  svbool_t swap = svcmpne (ptrue, svand_x (ptrue, sin_q, 1), 0);
  svfloat32_t sin = svsel (swap, sv_f32 (d->cos_start), sv_f32 (d->sin_start));
  svfloat32_t cos = svsel (swap, sv_f32 (d->sin_start), sv_f32 (d->cos_start));

  sin = svtmad (sin, sin_r2, 3);
  sin = svtmad (sin, sin_r2, 2);
  sin = svtmad (sin, sin_r2, 1);
  sin = svtmad (sin, sin_r2, 0);

  cos = svtmad (cos, cos_r2, 3);
  cos = svtmad (cos, cos_r2, 2);
  cos = svtmad (cos, cos_r2, 1);
  cos = svtmad (cos, cos_r2, 0);

  return svcreate2 (svmul_x (ptrue, sin_f, sin), svmul_x (ptrue, cos_f, cos));
}

/* Vectorised fallback for sincosf and cexpif for large input arguments.  */
static inline svfloat32x2_t
sv_sincos_fallback (svfloat32_t x, svbool_t special, const struct trig_data *d)
{
  special = svaclt (special, x, sv_f32 (INFINITY));

  svbool_t ptrue = svptrue_b32 ();

  /* Load some constants in quad-word chunks to minimise memory access.  */
  svfloat32_t negpio2_and_invpio2 = svld1rq (ptrue, &d->neg_pio2_1);

  /* n = rint(x/(pi/2)).  */
  svfloat32_t q = svmla_lane (sv_f32 (d->shift), x, negpio2_and_invpio2, 3);
  svfloat32_t n = svsub_x (ptrue, q, d->shift);

  /* r = x - n*(pi/2)  (range reduction into -pi/4 .. pi/4).  */
  svfloat32_t r = x;
  r = svmla_lane (r, n, negpio2_and_invpio2, 0);
  r = svmla_lane (r, n, negpio2_and_invpio2, 1);
  r = svmla_lane (r, n, negpio2_and_invpio2, 2);

  /* Reduce x into a quadrant and a remainder.  */
  svfloat32x2_t reduction = large_range_reduction (ptrue, x);

  /* Unpack the quadrant from the return struct.  */
  svuint32_t quadrant = svreinterpret_u32 (svget2 (reduction, 1));
  svfloat32_t remainder = svget2 (reduction, 0);

  r = svsel (special, remainder, r);
  svuint32_t sin_q = svsel (special, quadrant, svreinterpret_u32 (q));
  svuint32_t cos_q = svadd_x (ptrue, sin_q, 1);

  svfloat32_t sin_f = svtssel (r, sin_q);
  svfloat32_t cos_f = svtssel (r, cos_q);

  svfloat32_t sin_r2 = svtsmul (r, sin_q);
  svfloat32_t cos_r2 = svtsmul (r, cos_q);

  /* Manually selecting the starting value saves a redundant svtmad.  */
  svbool_t swap = svcmpne (ptrue, svand_x (ptrue, sin_q, 1), 0);
  svfloat32_t sin = svsel (swap, sv_f32 (d->cos_start), sv_f32 (d->sin_start));
  svfloat32_t cos = svsel (swap, sv_f32 (d->sin_start), sv_f32 (d->cos_start));

  /* sin(r) and cos(r) poly approx.  */
  sin = svtmad (sin, sin_r2, 3);
  sin = svtmad (sin, sin_r2, 2);
  sin = svtmad (sin, sin_r2, 1);
  sin = svtmad (sin, sin_r2, 0);
  sin = svmul_x (ptrue, sin_f, sin);

  cos = svtmad (cos, cos_r2, 3);
  cos = svtmad (cos, cos_r2, 2);
  cos = svtmad (cos, cos_r2, 1);
  cos = svtmad (cos, cos_r2, 0);
  cos = svmul_x (ptrue, cos_f, cos);

  return svcreate2 (sin, cos);
}
