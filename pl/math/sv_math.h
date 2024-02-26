/*
 * Wrapper functions for SVE ACLE.
 *
 * Copyright (c) 2019-2024, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#ifndef SV_MATH_H
#define SV_MATH_H

#ifndef WANT_VMATH
/* Enable the build of vector math code.  */
# define WANT_VMATH 1
#endif

#if WANT_VMATH

# include <arm_sve.h>
# include <stdbool.h>

# include "math_config.h"

/* Double precision.  */
static inline svint64_t
sv_s64 (int64_t x)
{
  return svdup_s64 (x);
}

static inline svuint64_t
sv_u64 (uint64_t x)
{
  return svdup_u64 (x);
}

static inline svfloat64_t
sv_f64 (double x)
{
  return svdup_f64 (x);
}

static inline svfloat64_t
sv_call_f64 (double (*f) (double), svfloat64_t x, svfloat64_t y, svbool_t cmp)
{
  svbool_t p = svpfirst (cmp, svpfalse ());
  while (svptest_any (cmp, p))
    {
      double elem = svclastb (p, 0, x);
      elem = (*f) (elem);
      svfloat64_t y2 = sv_f64 (elem);
      y = svsel (p, y2, y);
      p = svpnext_b64 (cmp, p);
    }
  return y;
}

static inline svfloat64_t
sv_call2_f64 (double (*f) (double, double), svfloat64_t x1, svfloat64_t x2,
	      svfloat64_t y, svbool_t cmp)
{
  svbool_t p = svpfirst (cmp, svpfalse ());
  while (svptest_any (cmp, p))
    {
      double elem1 = svclastb (p, 0, x1);
      double elem2 = svclastb (p, 0, x2);
      double ret = (*f) (elem1, elem2);
      svfloat64_t y2 = sv_f64 (ret);
      y = svsel (p, y2, y);
      p = svpnext_b64 (cmp, p);
    }
  return y;
}

static inline svuint64_t
sv_mod_n_u64_x (svbool_t pg, svuint64_t x, uint64_t y)
{
  svuint64_t q = svdiv_x (pg, x, y);
  return svmls_x (pg, x, q, y);
}

/* Single precision.  */
static inline svint32_t
sv_s32 (int32_t x)
{
  return svdup_s32 (x);
}

static inline svuint32_t
sv_u32 (uint32_t x)
{
  return svdup_u32 (x);
}

static inline svfloat32_t
sv_f32 (float x)
{
  return svdup_f32 (x);
}

static inline svfloat32_t
sv_call_f32 (float (*f) (float), svfloat32_t x, svfloat32_t y, svbool_t cmp)
{
  svbool_t p = svpfirst (cmp, svpfalse ());
  while (svptest_any (cmp, p))
    {
      float elem = svclastb (p, 0, x);
      elem = (*f) (elem);
      svfloat32_t y2 = sv_f32 (elem);
      y = svsel (p, y2, y);
      p = svpnext_b32 (cmp, p);
    }
  return y;
}

static inline svfloat32_t
sv_call2_f32 (float (*f) (float, float), svfloat32_t x1, svfloat32_t x2,
	      svfloat32_t y, svbool_t cmp)
{
  svbool_t p = svpfirst (cmp, svpfalse ());
  while (svptest_any (cmp, p))
    {
      float elem1 = svclastb (p, 0, x1);
      float elem2 = svclastb (p, 0, x2);
      float ret = (*f) (elem1, elem2);
      svfloat32_t y2 = sv_f32 (ret);
      y = svsel (p, y2, y);
      p = svpnext_b32 (cmp, p);
    }
  return y;
}

/* Streaming Compatible Gather.
   Iterates backwards over LUT indexes to load scalar values, then uses svinsr
   to prepend scalar to the front of a blank return vector.  */
static inline svuint64_t
sc_lookup_u64 (svuint64_t index, const uint64_t *data) SC_ATTR
{
  uint64_t ret_arr[svcntd ()];
  svst1 (svptrue_b64 (), ret_arr, index);
  svuint64_t ret = svdup_n_u64 (ret_arr[svcntd () - 1]);
  for (int i = svcntd () - 2; i >= 0; i--)
    ret = svinsr (ret, data[ret_arr[i]]);
  return ret;
}

static inline svfloat64_t
sc_lookup_f64 (svuint64_t index, const double *data)
{
  uint64_t idx_arr[svcntd ()];
  svst1 (svptrue_b64 (), idx_arr, index);
  svfloat64_t ret = svdup_n_f64 (idx_arr[svcntd () - 1]);
  for (int i = svcntd () - 2; i >= 0; i--)
    ret = svinsr (ret, data[idx_arr[i]]);
  return ret;
}

static inline svfloat32_t
sc_lookup_f32 (svuint32_t index, const float *data)
{
  uint32_t idx_arr[svcntw ()];
  svst1 (svptrue_b32 (), idx_arr, index);
  svfloat32_t ret = svdup_n_f32 (idx_arr[svcntw () - 1]);
  for (int i = svcntw () - 2; i >= 0; i--)
    ret = svinsr (ret, data[idx_arr[i]]);
  return ret;
}

static inline void
sc_lookup2_f64 (svuint64_t index, svfloat64_t *x0, svfloat64_t *x1,
		const double *data)
{
  uint64_t idx_arr[svcntd ()];
  svst1 (svptrue_b64 (), idx_arr, index);
  uint64_t idx = idx_arr[svcntd () - 1];
  *x0 = svdup_f64 (data[idx]);
  *x1 = svdup_f64 (data[idx + 1]);
  for (int i = svcntd () - 2; i >= 0; i--)
    {
      idx = idx_arr[i];
      *x0 = svinsr_n_f64 (*x0, data[idx]);
      *x1 = svinsr_n_f64 (*x1, data[idx + 1]);
    }
}

static inline void
sc_lookup2_f32 (svuint32_t index, svfloat32_t *x0, svfloat32_t *x1,
		const float *data)
{
  uint32_t idx_arr[svcntw ()];
  svst1 (svptrue_b32 (), idx_arr, index);
  uint32_t idx = idx_arr[svcntw () - 1];
  *x0 = svdup_f32 (data[idx]);
  *x1 = svdup_f32 (data[idx + 1]);
  for (int i = svcntw () - 2; i >= 0; i--)
    {
      idx = idx_arr[i];
      *x0 = svinsr_n_f32 (*x0, data[idx]);
      *x1 = svinsr_n_f32 (*x1, data[idx + 1]);
    }
}

#endif
#endif
