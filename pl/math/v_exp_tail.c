/*
 * Double-precision vector e^(x+tail) function.
 *
 * Copyright (c) 2019-2023, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "v_math.h"
#include "math_config.h"
#include "v_exp_tail.h"

#define C1 v_f64 (C1_scal)
#define C2 v_f64 (C2_scal)
#define C3 v_f64 (C3_scal)
#define InvLn2 v_f64 (InvLn2_scal)
#define Ln2hi v_f64 (Ln2hi_scal)
#define Ln2lo v_f64 (Ln2lo_scal)

#define IndexMask v_u64 (IndexMask_scal)
#define Shift v_f64 (Shift_scal)
#define Thres v_f64 (Thres_scal)

VPCS_ATTR
static float64x2_t
specialcase (float64x2_t s, float64x2_t y, float64x2_t n)
{
  float64x2_t absn = vabsq_f64 (n);

  /* 2^(n/N) may overflow, break it up into s1*s2.  */
  uint64x2_t b = v_cond_u64 (n <= v_f64 (0.0)) & v_u64 (0x6000000000000000);
  float64x2_t s1 = v_as_f64_u64 (v_u64 (0x7000000000000000) - b);
  float64x2_t s2
    = v_as_f64_u64 (v_as_u64_f64 (s) - v_u64 (0x3010000000000000) + b);
  uint64x2_t cmp = v_cond_u64 (absn > v_f64 (1280.0 * N));
  float64x2_t r1 = s1 * s1;
  float64x2_t r0 = v_fma_f64 (y, s2, s2) * s1;
  return v_as_f64_u64 ((cmp & v_as_u64_f64 (r1)) | (~cmp & v_as_u64_f64 (r0)));
}

VPCS_ATTR
float64x2_t
__v_exp_tail (float64x2_t x, float64x2_t xtail)
{
  float64x2_t n, r, s, y, z;
  uint64x2_t cmp, u, e, i;

  cmp = v_cond_u64 (vabsq_f64 (x) > Thres);

  /* n = round(x/(ln2/N)).  */
  z = v_fma_f64 (x, InvLn2, Shift);
  u = v_as_u64_f64 (z);
  n = z - Shift;

  /* r = x - n*ln2/N.  */
  r = x;
  r = v_fma_f64 (-Ln2hi, n, r);
  r = v_fma_f64 (-Ln2lo, n, r);

  e = u << (52 - V_EXP_TAIL_TABLE_BITS);
  i = u & IndexMask;

  /* y = tail + exp(r) - 1 ~= r + C1 r^2 + C2 r^3 + C3 r^4.  */
  y = v_fma_f64 (C3, r, C2);
  y = v_fma_f64 (y, r, C1);
  y = v_fma_f64 (y, r, v_f64 (1.0));
  y = v_fma_f64 (y, r, xtail);

  /* s = 2^(n/N).  */
  u = v_lookup_u64 (Tab, i);
  s = v_as_f64_u64 (u + e);

  if (unlikely (v_any_u64 (cmp)))
    return specialcase (s, y, n);
  return v_fma_f64 (y, s, s);
}
