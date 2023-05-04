/*
 * Single-precision vector 2^x function.
 *
 * Copyright (c) 2019-2023, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "mathlib.h"
#include "v_math.h"

static const float Poly[] = {
  /* maxerr: 1.962 ulp.  */
  0x1.59977ap-10f,
  0x1.3ce9e4p-7f,
  0x1.c6bd32p-5f,
  0x1.ebf9bcp-3f,
  0x1.62e422p-1f,
};
#define C0 v_f32 (Poly[0])
#define C1 v_f32 (Poly[1])
#define C2 v_f32 (Poly[2])
#define C3 v_f32 (Poly[3])
#define C4 v_f32 (Poly[4])

#define Shift v_f32 (0x1.8p23f)

#if WANT_SIMD_EXCEPT

#define TinyBound 0x20000000 /* asuint (0x1p-63).  */
#define BigBound 0x42800000  /* asuint (0x1p6).  */

static float32x4_t VPCS_ATTR NOINLINE
specialcase (float32x4_t x, float32x4_t y, uint32x4_t cmp)
{
  /* If fenv exceptions are to be triggered correctly, fall back to the scalar
     routine to special lanes.  */
  return v_call_f32 (exp2f, x, y, cmp);
}

#else

static float32x4_t VPCS_ATTR NOINLINE
specialcase (float32x4_t poly, float32x4_t n, uint32x4_t e, float32x4_t absn,
	     uint32x4_t cmp1, float32x4_t scale)
{
  /* 2^n may overflow, break it up into s1*s2.  */
  uint32x4_t b = v_cond_u32 (n <= v_f32 (0.0f)) & v_u32 (0x82000000);
  float32x4_t s1 = vreinterpretq_f32_u32 (v_u32 (0x7f000000) + b);
  float32x4_t s2 = vreinterpretq_f32_u32 (e - b);
  uint32x4_t cmp2 = v_cond_u32 (absn > v_f32 (192.0f));
  uint32x4_t r2 = vreinterpretq_u32_f32 (s1 * s1);
  uint32x4_t r1 = vreinterpretq_u32_f32 (v_fma_f32 (poly, s2, s2) * s1);
  /* Similar to r1 but avoids double rounding in the subnormal range.  */
  uint32x4_t r0 = vreinterpretq_u32_f32 (v_fma_f32 (poly, scale, scale));
  return vreinterpretq_f32_u32 ((cmp2 & r2) | (~cmp2 & cmp1 & r1)
				| (~cmp1 & r0));
}

#endif

float32x4_t VPCS_ATTR V_NAME (exp2f) (float32x4_t x)
{
  float32x4_t n, r, r2, scale, p, q, poly;
  uint32x4_t cmp, e;

#if WANT_SIMD_EXCEPT
  cmp = v_cond_u32 ((vreinterpretq_u32_f32 (x) & 0x7fffffff) - TinyBound
		    >= BigBound - TinyBound);
  float32x4_t xm = x;
  /* If any lanes are special, mask them with 1 and retain a copy of x to allow
     specialcase to fix special lanes later. This is only necessary if fenv
     exceptions are to be triggered correctly.  */
  if (unlikely (v_any_u32 (cmp)))
    x = v_sel_f32 (cmp, v_f32 (1), x);
#endif

    /* exp2(x) = 2^n (1 + poly(r)), with 1 + poly(r) in [1/sqrt(2),sqrt(2)]
       x = n + r, with r in [-1/2, 1/2].  */
#if 0
  float32x4_t z;
  z = x + Shift;
  n = z - Shift;
  r = x - n;
  e = vreinterpretq_u32_f32 (z) << 23;
#else
  n = v_round_f32 (x);
  r = x - n;
  e = vreinterpretq_u32_s32 (v_round_s32 (x)) << 23;
#endif
  scale = vreinterpretq_f32_u32 (e + v_u32 (0x3f800000));

#if !WANT_SIMD_EXCEPT
  float32x4_t absn = v_abs_f32 (n);
  cmp = v_cond_u32 (absn > v_f32 (126.0f));
#endif

  r2 = r * r;
  p = v_fma_f32 (C0, r, C1);
  q = v_fma_f32 (C2, r, C3);
  q = v_fma_f32 (p, r2, q);
  p = C4 * r;
  poly = v_fma_f32 (q, r2, p);

  if (unlikely (v_any_u32 (cmp)))
#if WANT_SIMD_EXCEPT
    return specialcase (xm, v_fma_f32 (poly, scale, scale), cmp);
#else
    return specialcase (poly, n, e, absn, cmp, scale);
#endif

  return v_fma_f32 (poly, scale, scale);
}
VPCS_ALIAS
