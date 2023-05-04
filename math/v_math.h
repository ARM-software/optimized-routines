/*
 * Vector math abstractions.
 *
 * Copyright (c) 2019-2023, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#ifndef _V_MATH_H
#define _V_MATH_H

#ifndef WANT_VMATH
/* Enable the build of vector math code.  */
# define WANT_VMATH 1
#endif
#if WANT_VMATH

#if VPCS && __aarch64__
#define V_NAME(x) __vn_##x
#define VPCS_ATTR __attribute__ ((aarch64_vector_pcs))
#else
#define V_NAME(x) __v_##x
#endif

#ifndef VPCS_ATTR
#define VPCS_ATTR
#endif
#ifndef VPCS_ALIAS
#define VPCS_ALIAS
#endif

#include <stdint.h>
#include "math_config.h"

/* reinterpret as type1 from type2.  */
static inline uint32_t
as_u32_f32 (float x)
{
  union
  {
    float f;
    uint32_t u;
  } r = {x};
  return r.u;
}
static inline float
as_f32_u32 (uint32_t x)
{
  union
  {
    uint32_t u;
    float f;
  } r = {x};
  return r.f;
}
static inline int32_t
as_s32_u32 (uint32_t x)
{
  union
  {
    uint32_t u;
    int32_t i;
  } r = {x};
  return r.i;
}
static inline uint32_t
as_u32_s32 (int32_t x)
{
  union
  {
    int32_t i;
    uint32_t u;
  } r = {x};
  return r.u;
}
static inline uint64_t
as_u64_f64 (double x)
{
  union
  {
    double f;
    uint64_t u;
  } r = {x};
  return r.u;
}
static inline double
as_f64_u64 (uint64_t x)
{
  union
  {
    uint64_t u;
    double f;
  } r = {x};
  return r.f;
}
static inline int64_t
as_s64_u64 (uint64_t x)
{
  union
  {
    uint64_t u;
    int64_t i;
  } r = {x};
  return r.i;
}
static inline uint64_t
as_u64_s64 (int64_t x)
{
  union
  {
    int64_t i;
    uint64_t u;
  } r = {x};
  return r.u;
}

#if __aarch64__
#include <arm_neon.h>

static inline int
v_lanes32 (void)
{
  return 4;
}

static inline float32x4_t
v_f32 (float x)
{
  return (float32x4_t){x, x, x, x};
}
static inline uint32x4_t
v_u32 (uint32_t x)
{
  return (uint32x4_t){x, x, x, x};
}
static inline int32x4_t
v_s32 (int32_t x)
{
  return (int32x4_t){x, x, x, x};
}
static inline float
v_get_f32 (float32x4_t x, int i)
{
  return x[i];
}
static inline uint32_t
v_get_u32 (uint32x4_t x, int i)
{
  return x[i];
}
static inline int32_t
v_get_s32 (int32x4_t x, int i)
{
  return x[i];
}

static inline void
v_set_f32 (float32x4_t *x, int i, float v)
{
  (*x)[i] = v;
}
static inline void
v_set_u32 (uint32x4_t *x, int i, uint32_t v)
{
  (*x)[i] = v;
}
static inline void
v_set_s32 (int32x4_t *x, int i, int32_t v)
{
  (*x)[i] = v;
}

/* true if any elements of a v_cond result is non-zero.  */
static inline int
v_any_u32 (uint32x4_t x)
{
  /* assume elements in x are either 0 or -1u.  */
  return vpaddd_u64 (vreinterpretq_u64_u32 (x)) != 0;
}
static inline float32x4_t
v_abs_f32 (float32x4_t x)
{
  return vabsq_f32 (x);
}
static inline float32x4_t
v_fma_f32 (float32x4_t x, float32x4_t y, float32x4_t z)
{
  return vfmaq_f32 (z, x, y);
}
static inline float32x4_t
v_round_f32 (float32x4_t x)
{
  return vrndaq_f32 (x);
}
static inline int32x4_t
v_round_s32 (float32x4_t x)
{
  return vcvtaq_s32_f32 (x);
}
static inline float32x4_t
v_sel_f32 (uint32x4_t p, float32x4_t x, float32x4_t y)
{
  return vbslq_f32 (p, x, y);
}
/* convert to type1 from type2.  */
static inline float32x4_t
v_to_f32_s32 (int32x4_t x)
{
  return (float32x4_t){x[0], x[1], x[2], x[3]};
}
static inline float32x4_t
v_to_f32_u32 (uint32x4_t x)
{
  return (float32x4_t){x[0], x[1], x[2], x[3]};
}
static inline float32x4_t
v_lookup_f32 (const float *tab, uint32x4_t idx)
{
  return (float32x4_t){tab[idx[0]], tab[idx[1]], tab[idx[2]], tab[idx[3]]};
}
static inline uint32x4_t
v_lookup_u32 (const uint32_t *tab, uint32x4_t idx)
{
  return (uint32x4_t){tab[idx[0]], tab[idx[1]], tab[idx[2]], tab[idx[3]]};
}
static inline float32x4_t
v_call_f32 (float (*f) (float), float32x4_t x, float32x4_t y, uint32x4_t p)
{
  return (float32x4_t){p[0] ? f (x[0]) : y[0], p[1] ? f (x[1]) : y[1],
		       p[2] ? f (x[2]) : y[2], p[3] ? f (x[3]) : y[3]};
}
static inline float32x4_t
v_call2_f32 (float (*f) (float, float), float32x4_t x1, float32x4_t x2,
	     float32x4_t y, uint32x4_t p)
{
  return (float32x4_t){p[0] ? f (x1[0], x2[0]) : y[0],
		       p[1] ? f (x1[1], x2[1]) : y[1],
		       p[2] ? f (x1[2], x2[2]) : y[2],
		       p[3] ? f (x1[3], x2[3]) : y[3]};
}

static inline int
v_lanes64 (void)
{
  return 2;
}
static inline float64x2_t
v_f64 (double x)
{
  return (float64x2_t){x, x};
}
static inline uint64x2_t
v_u64 (uint64_t x)
{
  return (uint64x2_t){x, x};
}
static inline int64x2_t
v_s64 (int64_t x)
{
  return (int64x2_t){x, x};
}
static inline double
v_get_f64 (float64x2_t x, int i)
{
  return x[i];
}
static inline void
v_set_f64 (float64x2_t *x, int i, double v)
{
  (*x)[i] = v;
}
/* true if any elements of a v_cond result is non-zero.  */
static inline int
v_any_u64 (uint64x2_t x)
{
  /* assume elements in x are either 0 or -1u.  */
  return vpaddd_u64 (x) != 0;
}
static inline float64x2_t
v_abs_f64 (float64x2_t x)
{
  return vabsq_f64 (x);
}
static inline float64x2_t
v_fma_f64 (float64x2_t x, float64x2_t y, float64x2_t z)
{
  return vfmaq_f64 (z, x, y);
}
static inline float64x2_t
v_round_f64 (float64x2_t x)
{
  return vrndaq_f64 (x);
}
static inline int64x2_t
v_round_s64 (float64x2_t x)
{
  return vcvtaq_s64_f64 (x);
}
static inline float64x2_t
v_sel_f64 (uint64x2_t p, float64x2_t x, float64x2_t y)
{
  return vbslq_f64 (p, x, y);
}
/* convert to type1 from type2.  */
static inline float64x2_t
v_to_f64_s64 (int64x2_t x)
{
  return (float64x2_t){x[0], x[1]};
}
static inline float64x2_t
v_to_f64_u64 (uint64x2_t x)
{
  return (float64x2_t){x[0], x[1]};
}
static inline float64x2_t
v_lookup_f64 (const double *tab, uint64x2_t idx)
{
  return (float64x2_t){tab[idx[0]], tab[idx[1]]};
}
static inline uint64x2_t
v_lookup_u64 (const uint64_t *tab, uint64x2_t idx)
{
  return (uint64x2_t){tab[idx[0]], tab[idx[1]]};
}
static inline float64x2_t
v_call_f64 (double (*f) (double), float64x2_t x, float64x2_t y, uint64x2_t p)
{
  return (float64x2_t){p[0] ? f (x[0]) : y[0], p[1] ? f (x[1]) : y[1]};
}
#endif

#endif
#endif
