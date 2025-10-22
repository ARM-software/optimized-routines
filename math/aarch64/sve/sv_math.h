/*
 * Wrapper functions for SVE ACLE.
 *
 * Copyright (c) 2019-2025, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#ifndef SV_MATH_H
#define SV_MATH_H

/* Enable SVE in this translation unit. Note, because this is 'pushed' in
   clang, any file including sv_math.h will have to pop it back off again by
   ending the source file with CLOSE_SVE_ATTR. It is important that sv_math.h
   is included first so that all functions have the target attribute.  */
#ifdef __clang__
# pragma clang attribute push(__attribute__((target("sve"))),                \
			       apply_to = any(function))
# define CLOSE_SVE_ATTR _Pragma("clang attribute pop")
#else
# pragma GCC target("+sve")
# define CLOSE_SVE_ATTR
#endif

#include <arm_sve.h>
#include <stdbool.h>

#include "math_config.h"

#define SV_NAME_F1(fun) _ZGVsMxv_##fun##f
#define SV_NAME_D1(fun) _ZGVsMxv_##fun
#define SV_NAME_F2(fun) _ZGVsMxvv_##fun##f
#define SV_NAME_D2(fun) _ZGVsMxvv_##fun
#define SV_NAME_F1_L1(fun) _ZGVsMxvl4_##fun##f
#define SV_NAME_D1_L1(fun) _ZGVsMxvl8_##fun
#define SV_NAME_F1_L2(fun) _ZGVsMxvl4l4_##fun##f

static inline void
svstr_p (uint8_t *dst, svbool_t p)
{
  /* Predicate STR does not currently have an intrinsic.  */
  __asm__("str %0, [%x1]\n" : : "Upa"(p), "r"(dst) : "memory");
}

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
  /* Buffer size corresponds to maximum possible vector length.  */
  double tmp[32];
  uint8_t pg_bits[32];
  svstr_p (pg_bits, cmp);
  svst1 (svptrue_b64 (), tmp, svsel (cmp, x, y));

  for (int i = 0; i < svcntd (); i++)
    {
      if (pg_bits[i] & 1)
	{
	  tmp[i] = f (tmp[i]);
	}
    }
  return svld1 (svptrue_b64 (), tmp);
}

static inline svfloat64_t
sv_call2_f64 (double (*f) (double, double), svfloat64_t x1, svfloat64_t x2,
	      svfloat64_t y, svbool_t cmp)
{
  /* Buffer size corresponds to maximum possible vector length.  */
  double tmp1[32], tmp2[32];
  uint8_t pg_bits[32];
  svstr_p (pg_bits, cmp);
  svst1 (svptrue_b64 (), tmp1, svsel (cmp, x1, y));
  svst1 (cmp, tmp2, x2);

  for (int i = 0; i < svcntd (); i++)
    {
      if (pg_bits[i] & 1)
	{
	  tmp1[i] = f (tmp1[i], tmp2[i]);
	}
    }
  return svld1 (svptrue_b64 (), tmp1);
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
  /* Buffer size corresponds to maximum vector length.  */
  float tmp[64];
  /* 32, not 64, is correct for pg_bits because each bit of pg_bits maps to 1
     byte of the vector, so a uint8_t indicates predication of two floats.  */
  uint8_t pg_bits[32];
  svstr_p (pg_bits, cmp);
  svst1 (svptrue_b32 (), tmp, svsel (cmp, x, y));

  for (int i = 0; i < svcntd (); i++)
    {
      uint8_t p = pg_bits[i];
      if (p & 1)
	{
	  tmp[i * 2] = f (tmp[i * 2]);
	}
      if (p & (1 << 4))
	{
	  tmp[i * 2 + 1] = f (tmp[i * 2 + 1]);
	}
    }
  return svld1 (svptrue_b32 (), tmp);
}

static inline svfloat32_t
sv_call2_f32 (float (*f) (float, float), svfloat32_t x1, svfloat32_t x2,
	      svfloat32_t y, svbool_t cmp)
{
  /* Buffer size corresponds to maximum vector length.  */
  float tmp1[64], tmp2[64];
  uint8_t pg_bits[32];
  svstr_p (pg_bits, cmp);
  svst1 (svptrue_b32 (), tmp1, svsel (cmp, x1, y));
  svst1 (cmp, tmp2, x2);

  for (int i = 0; i < svcntd (); i++)
    {
      uint8_t p = pg_bits[i];
      if (p & 1)
	{
	  tmp1[i * 2] = f (tmp1[i * 2], tmp2[i * 2]);
	}
      if (p & (1 << 4))
	{
	  tmp1[i * 2 + 1] = f (tmp1[i * 2 + 1], tmp2[i * 2 + 1]);
	}
    }
  return svld1 (svptrue_b32 (), tmp1);
}
#endif
