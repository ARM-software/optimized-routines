/*
 * Single-precision vector e^x function.
 *
 * Copyright (c) 2019-2023, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "mathlib.h"
#include "v_math.h"

static const float Poly[] = {
  /* maxerr: 1.45358 +0.5 ulp.  */
  0x1.0e4020p-7f,
  0x1.573e2ep-5f,
  0x1.555e66p-3f,
  0x1.fffdb6p-2f,
  0x1.ffffecp-1f,
};
#define C0 v_f32 (Poly[0])
#define C1 v_f32 (Poly[1])
#define C2 v_f32 (Poly[2])
#define C3 v_f32 (Poly[3])
#define C4 v_f32 (Poly[4])

#define Shift v_f32 (0x1.8p23f)
#define InvLn2 v_f32 (0x1.715476p+0f)
#define Ln2hi v_f32 (0x1.62e4p-1f)
#define Ln2lo v_f32 (0x1.7f7d1cp-20f)

#if WANT_SIMD_EXCEPT

#define TinyBound 0x20000000 /* asuint (0x1p-63).  */
#define BigBound 0x42800000  /* asuint (0x1p6).  */

static float32x4_t VPCS_ATTR NOINLINE
specialcase (float32x4_t x, float32x4_t y, uint32x4_t cmp)
{
  /* If fenv exceptions are to be triggered correctly, fall back to the scalar
     routine to special lanes.  */
  return v_call_f32 (expf, x, y, cmp);
}

#else

static float32x4_t VPCS_ATTR NOINLINE
specialcase (float32x4_t poly, float32x4_t n, uint32x4_t e, float32x4_t absn,
	     uint32x4_t cmp1, float32x4_t scale)
{
  /* 2^n may overflow, break it up into s1*s2.  */
  uint32x4_t b = (n <= v_f32 (0.0f)) & v_u32 (0x82000000);
  float32x4_t s1 = vreinterpretq_f32_u32 (v_u32 (0x7f000000) + b);
  float32x4_t s2 = vreinterpretq_f32_u32 (e - b);
  uint32x4_t cmp2 = absn > v_f32 (192.0f);
  uint32x4_t r2 = vreinterpretq_u32_f32 (s1 * s1);
  uint32x4_t r1 = vreinterpretq_u32_f32 (v_fma_f32 (poly, s2, s2) * s1);
  /* Similar to r1 but avoids double rounding in the subnormal range.  */
  uint32x4_t r0 = vreinterpretq_u32_f32 (v_fma_f32 (poly, scale, scale));
  return vreinterpretq_f32_u32 ((cmp2 & r2) | (~cmp2 & cmp1 & r1)
				| (~cmp1 & r0));
}

#endif

float32x4_t VPCS_ATTR V_NAME (expf) (float32x4_t x)
{
  float32x4_t n, r, r2, scale, p, q, poly, z;
  uint32x4_t cmp, e;

#if WANT_SIMD_EXCEPT
  cmp = (vreinterpretq_u32_f32 (x) & 0x7fffffff) - TinyBound
	>= BigBound - TinyBound;
  float32x4_t xm = x;
  /* If any lanes are special, mask them with 1 and retain a copy of x to allow
     specialcase to fix special lanes later. This is only necessary if fenv
     exceptions are to be triggered correctly.  */
  if (unlikely (v_any_u32 (cmp)))
    x = v_sel_f32 (cmp, v_f32 (1), x);
#endif

    /* exp(x) = 2^n (1 + poly(r)), with 1 + poly(r) in [1/sqrt(2),sqrt(2)]
       x = ln2*n + r, with r in [-ln2/2, ln2/2].  */
#if 1
  z = v_fma_f32 (x, InvLn2, Shift);
  n = z - Shift;
  r = v_fma_f32 (n, -Ln2hi, x);
  r = v_fma_f32 (n, -Ln2lo, r);
  e = vreinterpretq_u32_f32 (z) << 23;
#else
  z = x * InvLn2;
  n = v_round_f32 (z);
  r = v_fma_f32 (n, -Ln2hi, x);
  r = v_fma_f32 (n, -Ln2lo, r);
  e = vreinterpretq_u32_s32 (v_round_s32 (z)) << 23;
#endif
  scale = vreinterpretq_f32_u32 (e + v_u32 (0x3f800000));

#if !WANT_SIMD_EXCEPT
  float32x4_t absn = v_abs_f32 (n);
  cmp = absn > v_f32 (126.0f);
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
