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

#if __aarch64__
#define V_NAME(x) __vn_##x
#define VPCS_ATTR __attribute__ ((aarch64_vector_pcs))
#else
#error "Cannot build without AArch64"
#endif

#include <stdint.h>
#include "math_config.h"

typedef float f32_t;
typedef uint32_t u32_t;
typedef int32_t s32_t;
typedef double f64_t;
typedef uint64_t u64_t;
typedef int64_t s64_t;

/* reinterpret as type1 from type2.  */
static inline u32_t
as_u32_f32 (f32_t x)
{
  union { f32_t f; u32_t u; } r = {x};
  return r.u;
}
static inline f32_t
as_f32_u32 (u32_t x)
{
  union { u32_t u; f32_t f; } r = {x};
  return r.f;
}
static inline s32_t
as_s32_u32 (u32_t x)
{
  union { u32_t u; s32_t i; } r = {x};
  return r.i;
}
static inline u32_t
as_u32_s32 (s32_t x)
{
  union { s32_t i; u32_t u; } r = {x};
  return r.u;
}
static inline u64_t
as_u64_f64 (f64_t x)
{
  union { f64_t f; u64_t u; } r = {x};
  return r.u;
}
static inline f64_t
as_f64_u64 (u64_t x)
{
  union { u64_t u; f64_t f; } r = {x};
  return r.f;
}
static inline s64_t
as_s64_u64 (u64_t x)
{
  union { u64_t u; s64_t i; } r = {x};
  return r.i;
}
static inline u64_t
as_u64_s64 (s64_t x)
{
  union { s64_t i; u64_t u; } r = {x};
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
v_f32 (f32_t x)
{
  return (float32x4_t){x, x, x, x};
}
static inline uint32x4_t
v_u32 (u32_t x)
{
  return (uint32x4_t){x, x, x, x};
}
static inline int32x4_t
v_s32 (s32_t x)
{
  return (int32x4_t){x, x, x, x};
}

static inline f32_t
v_get_f32 (float32x4_t x, int i)
{
  return x[i];
}
static inline u32_t
v_get_u32 (uint32x4_t x, int i)
{
  return x[i];
}
static inline s32_t
v_get_s32 (int32x4_t x, int i)
{
  return x[i];
}

static inline void
v_set_f32 (float32x4_t *x, int i, f32_t v)
{
  (*x)[i] = v;
}
static inline void
v_set_u32 (uint32x4_t *x, int i, u32_t v)
{
  (*x)[i] = v;
}
static inline void
v_set_s32 (int32x4_t *x, int i, s32_t v)
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
/* to wrap the result of relational operators.  */
static inline uint32x4_t
v_cond_u32 (uint32x4_t x)
{
  return x;
}
static inline float32x4_t
v_abs_f32 (float32x4_t x)
{
  return vabsq_f32 (x);
}
static inline uint32x4_t
v_bsl_u32 (uint32x4_t m, uint32x4_t x, uint32x4_t y)
{
  return vbslq_u32 (m, x, y);
}
static inline uint32x4_t
v_cagt_f32 (float32x4_t x, float32x4_t y)
{
  return vcagtq_f32 (x, y);
}
/* to wrap |x| >= |y|.  */
static inline uint32x4_t
v_cage_f32 (float32x4_t x, float32x4_t y)
{
  return vcageq_f32 (x, y);
}
static inline uint32x4_t
v_calt_f32 (float32x4_t x, float32x4_t y)
{
  return vcaltq_f32 (x, y);
}
static inline float32x4_t
v_div_f32 (float32x4_t x, float32x4_t y)
{
  return vdivq_f32 (x, y);
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
static inline uint32x4_t
v_sel_u32 (uint32x4_t p, uint32x4_t x, uint32x4_t y)
{
  return vbslq_u32 (p, x, y);
}
static inline float32x4_t
v_sqrt_f32 (float32x4_t x)
{
  return vsqrtq_f32 (x);
}
/* convert to type1 from type2.  */
static inline float32x4_t
v_to_f32_s32 (int32x4_t x)
{
  return (float32x4_t){x[0], x[1], x[2], x[3]};
}
static inline int32x4_t
v_to_s32_f32 (float32x4_t x)
{
  return vcvtq_s32_f32 (x);
}
static inline float32x4_t
v_to_f32_u32 (uint32x4_t x)
{
  return (float32x4_t){x[0], x[1], x[2], x[3]};
}
/* reinterpret as type1 from type2.  */
static inline uint32x4_t
v_as_u32_f32 (float32x4_t x)
{
  union
  {
    float32x4_t f;
    uint32x4_t u;
  } r = {x};
  return r.u;
}
static inline int32x4_t
v_as_s32_f32 (float32x4_t x)
{
  union
  {
    float32x4_t f;
    int32x4_t u;
  } r = {x};
  return r.u;
}
static inline float32x4_t
v_as_f32_u32 (uint32x4_t x)
{
  union
  {
    uint32x4_t u;
    float32x4_t f;
  } r = {x};
  return r.f;
}
static inline int32x4_t
v_as_s32_u32 (uint32x4_t x)
{
  union
  {
    uint32x4_t u;
    int32x4_t i;
  } r = {x};
  return r.i;
}
static inline uint32x4_t
v_as_u32_s32 (int32x4_t x)
{
  union
  {
    int32x4_t i;
    uint32x4_t u;
  } r = {x};
  return r.u;
}
static inline float32x4_t
v_lookup_f32 (const f32_t *tab, uint32x4_t idx)
{
  return (float32x4_t){tab[idx[0]], tab[idx[1]], tab[idx[2]], tab[idx[3]]};
}
static inline uint32x4_t
v_lookup_u32 (const u32_t *tab, uint32x4_t idx)
{
  return (uint32x4_t){tab[idx[0]], tab[idx[1]], tab[idx[2]], tab[idx[3]]};
}
static inline float32x4_t
v_call_f32 (f32_t (*f) (f32_t), float32x4_t x, float32x4_t y, uint32x4_t p)
{
  return (float32x4_t){p[0] ? f (x[0]) : y[0], p[1] ? f (x[1]) : y[1],
		       p[2] ? f (x[2]) : y[2], p[3] ? f (x[3]) : y[3]};
}
static inline float32x4_t
v_call2_f32 (f32_t (*f) (f32_t, f32_t), float32x4_t x1, float32x4_t x2,
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
v_f64 (f64_t x)
{
  return (float64x2_t){x, x};
}
static inline uint64x2_t
v_u64 (u64_t x)
{
  return (uint64x2_t){x, x};
}
static inline int64x2_t
v_s64 (s64_t x)
{
  return (int64x2_t){x, x};
}
static inline f64_t
v_get_f64 (float64x2_t x, int i)
{
  return x[i];
}
static inline void
v_set_f64 (float64x2_t *x, int i, f64_t v)
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
/* true if all elements of a v_cond result is 1.  */
static inline int
v_all_u64 (uint64x2_t x)
{
  /* assume elements in x are either 0 or -1u.  */
  return vpaddd_s64 (vreinterpretq_s64_u64 (x)) == -2;
}
/* to wrap the result of relational operators.  */
static inline uint64x2_t
v_cond_u64 (uint64x2_t x)
{
  return x;
}
static inline float64x2_t
v_abs_f64 (float64x2_t x)
{
  return vabsq_f64 (x);
}
static inline uint64x2_t
v_bsl_u64 (uint64x2_t m, uint64x2_t x, uint64x2_t y)
{
  return vbslq_u64 (m, x, y);
}
static inline uint64x2_t
v_cagt_f64 (float64x2_t x, float64x2_t y)
{
  return vcagtq_f64 (x, y);
}
static inline float64x2_t
v_div_f64 (float64x2_t x, float64x2_t y)
{
  return vdivq_f64 (x, y);
}
static inline float64x2_t
v_fma_f64 (float64x2_t x, float64x2_t y, float64x2_t z)
{
  return vfmaq_f64 (z, x, y);
}
static inline float64x2_t
v_min_f64 (float64x2_t x, float64x2_t y)
{
  return vminq_f64(x, y);
}
static inline float64x2_t
v_round_f64 (float64x2_t x)
{
  return vrndaq_f64 (x);
}
static inline float64x2_t
v_sel_f64 (uint64x2_t p, float64x2_t x, float64x2_t y)
{
  return vbslq_f64 (p, x, y);
}
static inline uint64x2_t
v_sel_u64 (uint64x2_t p, uint64x2_t x, uint64x2_t y)
{
  return vbslq_u64 (p, x, y);
}
static inline float64x2_t
v_sqrt_f64 (float64x2_t x)
{
  return vsqrtq_f64 (x);
}
static inline int64x2_t
v_round_s64 (float64x2_t x)
{
  return vcvtaq_s64_f64 (x);
}
static inline uint64x2_t
v_trunc_u64 (float64x2_t x)
{
  return vcvtq_u64_f64 (x);
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
static inline int64x2_t
v_to_s64_f64 (float64x2_t x)
{
  return vcvtq_s64_f64 (x);
}
/* reinterpret as type1 from type2.  */
static inline uint64x2_t
v_as_u64_f64 (float64x2_t x)
{
  union
  {
    float64x2_t f;
    uint64x2_t u;
  } r = {x};
  return r.u;
}
static inline float64x2_t
v_as_f64_u64 (uint64x2_t x)
{
  union
  {
    uint64x2_t u;
    float64x2_t f;
  } r = {x};
  return r.f;
}
static inline int64x2_t
v_as_s64_u64 (uint64x2_t x)
{
  union
  {
    uint64x2_t u;
    int64x2_t i;
  } r = {x};
  return r.i;
}
static inline uint64x2_t
v_as_u64_s64 (int64x2_t x)
{
  union
  {
    int64x2_t i;
    uint64x2_t u;
  } r = {x};
  return r.u;
}
static inline float64x2_t
v_lookup_f64 (const f64_t *tab, uint64x2_t idx)
{
  return (float64x2_t){tab[idx[0]], tab[idx[1]]};
}
static inline uint64x2_t
v_lookup_u64 (const u64_t *tab, uint64x2_t idx)
{
  return (uint64x2_t){tab[idx[0]], tab[idx[1]]};
}
static inline float64x2_t
v_call_f64 (f64_t (*f) (f64_t), float64x2_t x, float64x2_t y, uint64x2_t p)
{
  return (float64x2_t){p[0] ? f (x[0]) : y[0], p[1] ? f (x[1]) : y[1]};
}
static inline float64x2_t
v_call2_f64 (f64_t (*f) (f64_t, f64_t), float64x2_t x1, float64x2_t x2,
	     float64x2_t y, uint64x2_t p)
{
  return (float64x2_t){p[0] ? f (x1[0], x2[0]) : y[0],
		       p[1] ? f (x1[1], x2[1]) : y[1]};
}
#endif

#endif
#endif
