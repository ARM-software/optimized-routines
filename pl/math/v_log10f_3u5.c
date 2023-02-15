/*
 * Single-precision vector log10 function.
 *
 * Copyright (c) 2020-2023, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "v_math.h"
#include "mathlib.h"
#include "pl_sig.h"
#include "pl_test.h"

#define P(i) v_f32 (__v_log10f_poly[i])

#define Ln2 v_f32 (0x1.62e43p-1f) /* 0x3f317218.  */
#define InvLn10 v_f32 (0x1.bcb7b2p-2f)
#define Min v_u32 (0x00800000)
#define Max v_u32 (0x7f800000)
#define Mask v_u32 (0x007fffff)
#define Off v_u32 (0x3f2aaaab) /* 0.666667.  */

VPCS_ATTR
NOINLINE static float32x4_t
specialcase (float32x4_t x, float32x4_t y, uint32x4_t cmp)
{
  /* Fall back to scalar code.  */
  return v_call_f32 (log10f, x, y, cmp);
}

/* Our fast implementation of v_log10f uses a similar approach as v_logf.
   With the same offset as v_logf (i.e., 2/3) it delivers about 3.3ulps with
   order 9. This is more efficient than using a low order polynomial computed in
   double precision.
   Maximum error: 3.305ulps (nearest rounding.)
   __v_log10f(0x1.555c16p+0) got 0x1.ffe2fap-4
			    want 0x1.ffe2f4p-4 -0.304916 ulp err 2.80492.  */
VPCS_ATTR
float32x4_t V_NAME_F1 (log10) (float32x4_t x)
{
  float32x4_t n, o, p, q, r, r2, y;
  uint32x4_t u, cmp;

  u = v_as_u32_f32 (x);
  cmp = u - Min >= Max - Min;

  /* x = 2^n * (1+r), where 2/3 < 1+r < 4/3.  */
  u -= Off;
  n = vcvtq_f32_s32 (v_as_s32_u32 (u) >> 23); /* signextend.  */
  u &= Mask;
  u += Off;
  r = v_as_f32_u32 (u) - v_f32 (1.0f);

  /* y = log10(1+r) + n*log10(2).  */
  r2 = r * r;
  /* (n*ln2 + r)*InvLn10 + r2*(P0 + r*P1 + r2*(P2 + r*P3 + r2*(P4 + r*P5 +
     r2*(P6+r*P7))).  */
  o = vfmaq_f32 (P (6), P (7), r);
  p = vfmaq_f32 (P (4), P (5), r);
  q = vfmaq_f32 (P (2), P (3), r);
  y = vfmaq_f32 (P (0), P (1), r);
  p = vfmaq_f32 (p, o, r2);
  q = vfmaq_f32 (q, p, r2);
  y = vfmaq_f32 (y, q, r2);
  /* Using p = Log10(2)*n + r*InvLn(10) is slightly faster
     but less accurate.  */
  p = vfmaq_f32 (r, Ln2, n);
  y = vfmaq_f32 (p * InvLn10, y, r2);

  if (unlikely (v_any_u32 (cmp)))
    return specialcase (x, y, cmp);
  return y;
}

PL_SIG (V, F, 1, log10, 0.01, 11.1)
PL_TEST_ULP (V_NAME_F1 (log10), 2.81)
PL_TEST_EXPECT_FENV_ALWAYS (V_NAME_F1 (log10))
PL_TEST_INTERVAL (V_NAME_F1 (log10), 0, 0xffff0000, 10000)
PL_TEST_INTERVAL (V_NAME_F1 (log10), 0x1p-4, 0x1p4, 500000)
