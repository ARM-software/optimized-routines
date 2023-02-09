/*
 * Wrapper functions for SVE ACLE.
 *
 * Copyright (c) 2019-2023, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#ifndef SV_MATH_H
#define SV_MATH_H

#ifndef WANT_VMATH
/* Enable the build of vector math code.  */
#define WANT_VMATH 1
#endif
#if WANT_VMATH

#if WANT_SVE_MATH
#define SV_SUPPORTED 1

#include <arm_sve.h>
#include <stdbool.h>

#include "math_config.h"

typedef float f32_t;
typedef uint32_t u32_t;
typedef int32_t s32_t;
typedef double f64_t;
typedef uint64_t u64_t;
typedef int64_t s64_t;

/* Double precision.  */
static inline svint64_t
sv_s64 (s64_t x)
{
  return svdup_n_s64 (x);
}

static inline svuint64_t
sv_u64 (u64_t x)
{
  return svdup_n_u64 (x);
}

static inline svfloat64_t
sv_f64 (f64_t x)
{
  return svdup_n_f64 (x);
}

static inline svfloat64_t
sv_fma_f64_x (svbool_t pg, svfloat64_t x, svfloat64_t y, svfloat64_t z)
{
  return svmla_f64_x (pg, z, x, y);
}

/* res = z + x * y with x scalar. */
static inline svfloat64_t
sv_fma_n_f64_x (svbool_t pg, f64_t x, svfloat64_t y, svfloat64_t z)
{
  return svmla_n_f64_x (pg, z, y, x);
}

static inline svint64_t
sv_as_s64_u64 (svuint64_t x)
{
  return svreinterpret_s64_u64 (x);
}

static inline svuint64_t
sv_as_u64_f64 (svfloat64_t x)
{
  return svreinterpret_u64_f64 (x);
}

static inline svfloat64_t
sv_as_f64_u64 (svuint64_t x)
{
  return svreinterpret_f64_u64 (x);
}

static inline svfloat64_t
sv_call_f64 (f64_t (*f) (f64_t), svfloat64_t x, svfloat64_t y, svbool_t cmp)
{
  svbool_t p = svpfirst (cmp, svpfalse ());
  while (svptest_any (cmp, p))
    {
      f64_t elem = svclastb_n_f64 (p, 0, x);
      elem = (*f) (elem);
      svfloat64_t y2 = svdup_n_f64 (elem);
      y = svsel_f64 (p, y2, y);
      p = svpnext_b64 (cmp, p);
    }
  return y;
}

static inline svfloat64_t
sv_call2_f64 (f64_t (*f) (f64_t, f64_t), svfloat64_t x1, svfloat64_t x2,
	      svfloat64_t y, svbool_t cmp)
{
  svbool_t p = svpfirst (cmp, svpfalse ());
  while (svptest_any (cmp, p))
    {
      f64_t elem1 = svclastb_n_f64 (p, 0, x1);
      f64_t elem2 = svclastb_n_f64 (p, 0, x2);
      f64_t ret = (*f) (elem1, elem2);
      svfloat64_t y2 = svdup_n_f64 (ret);
      y = svsel_f64 (p, y2, y);
      p = svpnext_b64 (cmp, p);
    }
  return y;
}

static inline svuint64_t
sv_mod_n_u64_x (svbool_t pg, svuint64_t x, u64_t y)
{
  svuint64_t q = svdiv_n_u64_x (pg, x, y);
  return svmls_n_u64_x (pg, x, q, y);
}

/* Single precision.  */
static inline svint32_t
sv_s32 (s32_t x)
{
  return svdup_n_s32 (x);
}

static inline svuint32_t
sv_u32 (u32_t x)
{
  return svdup_n_u32 (x);
}

static inline svfloat32_t
sv_f32 (f32_t x)
{
  return svdup_n_f32 (x);
}

static inline svfloat32_t
sv_fma_f32_x (svbool_t pg, svfloat32_t x, svfloat32_t y, svfloat32_t z)
{
  return svmla_f32_x (pg, z, x, y);
}

/* res = z + x * y with x scalar.  */
static inline svfloat32_t
sv_fma_n_f32_x (svbool_t pg, f32_t x, svfloat32_t y, svfloat32_t z)
{
  return svmla_n_f32_x (pg, z, y, x);
}

static inline svuint32_t
sv_as_u32_f32 (svfloat32_t x)
{
  return svreinterpret_u32_f32 (x);
}

static inline svuint32_t
sv_as_u32_s32 (svint32_t x)
{
  return svreinterpret_u32_s32 (x);
}

static inline svfloat32_t
sv_as_f32_u32 (svuint32_t x)
{
  return svreinterpret_f32_u32 (x);
}

static inline svfloat32_t
sv_as_f32_u64 (svuint64_t x)
{
  return svreinterpret_f32_u64 (x);
}

static inline svint32_t
sv_as_s32_u32 (svuint32_t x)
{
  return svreinterpret_s32_u32 (x);
}

static inline svfloat32_t
sv_call_f32 (f32_t (*f) (f32_t), svfloat32_t x, svfloat32_t y, svbool_t cmp)
{
  svbool_t p = svpfirst (cmp, svpfalse ());
  while (svptest_any (cmp, p))
    {
      f32_t elem = svclastb_n_f32 (p, 0, x);
      elem = (*f) (elem);
      svfloat32_t y2 = svdup_n_f32 (elem);
      y = svsel_f32 (p, y2, y);
      p = svpnext_b32 (cmp, p);
    }
  return y;
}

static inline svfloat32_t
sv_call2_f32 (f32_t (*f) (f32_t, f32_t), svfloat32_t x1, svfloat32_t x2,
	      svfloat32_t y, svbool_t cmp)
{
  svbool_t p = svpfirst (cmp, svpfalse ());
  while (svptest_any (cmp, p))
    {
      f32_t elem1 = svclastb_n_f32 (p, 0, x1);
      f32_t elem2 = svclastb_n_f32 (p, 0, x2);
      f32_t ret = (*f) (elem1, elem2);
      svfloat32_t y2 = svdup_n_f32 (ret);
      y = svsel_f32 (p, y2, y);
      p = svpnext_b32 (cmp, p);
    }
  return y;
}

#endif
#endif
#endif
