/*
 * Single-precision SVE pow and powr function helpers.
 *
 * Copyright (c) 2025-2026, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "powf_common.h"
#include "sv_math.h"

/* The following data is used in the SVE pow core computation
   and special case detection.  */
#define Tinvc __v_powf_data.invc
#define Tlogc __v_powf_data.logc
static const struct data
{
  double log_poly[4];
  double exp_poly[2], shift;
  float uflow_bound, oflow_bound, small_bound;
  uint32_t subnormal_bias, off;
} data = {
  .log_poly = {
    /* rel err: 1.5 * 2^-30.  */
    -0x1.6ff5daa3b3d7cp-2,
    0x1.ec81d03c01aebp-2,
    -0x1.71547bb43f101p-1,
    0x1.7154764a815cbp+0,
  },
  .exp_poly = {
    /* Approximate 2^r on [-1/128, 1/128] (FEXPA means N=64)
       with 1 + c1 * r + c2 * r^2 using Sollya's Remez algorithm.
       Relative approximation error: 1.78 * 2^-28.  */
    0x1.ebfbdfcf1ce17p-3,
    0x1.62e48521e5ca8p-1,
  },
  .shift = 0x1.800000000ffc0p+46,
  .uflow_bound = -150.0,
  .oflow_bound = 128.0,
  .small_bound = 0x1p-126f,
  .off = 0x3f35d000,
  .subnormal_bias = 0x0b800000, /* 23 << 23.  */
};

/* Check if x is an integer.  */
static inline svbool_t
svisint (svbool_t pg, svfloat32_t x)
{
  return svcmpeq (pg, svrintz_z (pg, x), x);
}

/* Check if x is real not integer valued.  */
static inline svbool_t
svisnotint (svbool_t pg, svfloat32_t x)
{
  return svcmpne (pg, svrintz_z (pg, x), x);
}

/* Check if x is an odd integer.  */
static inline svbool_t
svisodd (svbool_t pg, svfloat32_t x)
{
  svfloat32_t y = svmul_x (pg, x, 0.5f);
  return svisnotint (pg, y);
}

/* Check if zero, inf or nan.  */
static inline svbool_t
sv_zeroinfnan (svbool_t pg, svuint32_t i)
{
  return svcmpge (pg, svsub_x (pg, svadd_x (pg, i, i), 1),
		  2u * 0x7f800000 - 1);
}

/* Compute core for half of the lanes in double precision.  */
static inline svfloat64_t
sv_powf_core_ext (const svbool_t pg, svuint64_t i, svfloat64_t z, svint64_t k,
		  svfloat64_t y, svfloat64_t *pylogx,
		  const struct data *d)
{
  svfloat64_t invc = svld1_gather_index (pg, Tinvc, i);
  svfloat64_t logc = svld1_gather_index (pg, Tlogc, i);

  /* log2(x) = log1p(z/c-1)/ln2 + log2(c) + k.  */
  svfloat64_t r = svmla_x (pg, sv_f64 (-1.0), z, invc);
  svfloat64_t y0 = svadd_x (pg, logc, svcvt_f64_x (pg, k));

  /* Polynomial to approximate log1p(r)/ln2.  */
  svfloat64_t logx = sv_f64 (d->log_poly[0]);
  logx = svmad_x (pg, r, logx, sv_f64 (d->log_poly[1]));
  logx = svmad_x (pg, r, logx, sv_f64 (d->log_poly[2]));
  logx = svmad_x (pg, r, logx, sv_f64 (d->log_poly[3]));
  logx = svmad_x (pg, r, logx, y0);
  *pylogx = svmul_x (pg, y, logx);

  /* Evaluate exp2 (ylogx) using FEXPA-based approach.  */
  z = svadd_x (svptrue_b64 (), *pylogx, d->shift);
  svfloat64_t n = svsub_x (pg, z, d->shift);
  r = svsub_x (pg, *pylogx, n);

  svfloat64_t scale = svexpa (svreinterpret_u64 (z));

  svfloat64_t p = sv_f64 (d->exp_poly[0]);
  p = svmla_x (pg, sv_f64 (d->exp_poly[1]), p, r);
  p = svmla_x (pg, scale, p, svmul_x (svptrue_b64 (), scale, r));
  return p;
}

/* Widen vector to double precision and compute core on both halves of the
   vector. Lower cost of promotion by considering all lanes active.  */
static inline svfloat32_t
sv_powf_core (const svbool_t pg, svuint32_t i, svuint32_t iz, svint32_t k,
	      svfloat32_t y, svfloat32_t *pylogx, const struct data *d)
{
  const svbool_t ptrue = svptrue_b64 ();

  /* Unpack and promote input vectors (pg, y, z, i, k) into two
     in order to perform core computation in double precision.  */
  const svbool_t pg_lo = svunpklo (pg);
  const svbool_t pg_hi = svunpkhi (pg);
  svfloat64_t y_lo = svcvt_f64_x (
      ptrue, svreinterpret_f32 (svunpklo (svreinterpret_u32 (y))));
  svfloat64_t y_hi = svcvt_f64_x (
      ptrue, svreinterpret_f32 (svunpkhi (svreinterpret_u32 (y))));
  svfloat64_t z_lo = svcvt_f64_x (ptrue, svreinterpret_f32 (svunpklo (iz)));
  svfloat64_t z_hi = svcvt_f64_x (ptrue, svreinterpret_f32 (svunpkhi (iz)));
  svuint64_t i_lo = svunpklo (i);
  svuint64_t i_hi = svunpkhi (i);
  svint64_t k_lo = svunpklo (k);
  svint64_t k_hi = svunpkhi (k);

  /* Compute each part in double precision.  */
  svfloat64_t ylogx_lo, ylogx_hi;
  svfloat64_t lo
      = sv_powf_core_ext (pg_lo, i_lo, z_lo, k_lo, y_lo, &ylogx_lo, d);
  svfloat64_t hi
      = sv_powf_core_ext (pg_hi, i_hi, z_hi, k_hi, y_hi, &ylogx_hi, d);

  /* Convert back to single-precision and interleave.  */
  svfloat32_t ylogx_lo_32 = svcvt_f32_x (ptrue, ylogx_lo);
  svfloat32_t ylogx_hi_32 = svcvt_f32_x (ptrue, ylogx_hi);
  *pylogx = svuzp1 (ylogx_lo_32, ylogx_hi_32);
  svfloat32_t lo_32 = svcvt_f32_x (ptrue, lo);
  svfloat32_t hi_32 = svcvt_f32_x (ptrue, hi);
  return svuzp1 (lo_32, hi_32);
}
