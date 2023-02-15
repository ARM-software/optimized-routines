/*
 * Double-precision vector log2 function.
 *
 * Copyright (c) 2022-2023, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "v_math.h"
#include "include/mathlib.h"
#include "pl_sig.h"
#include "pl_test.h"

#define InvLn2 v_f64 (0x1.71547652b82fep0)
#define N (1 << V_LOG2_TABLE_BITS)
#define OFF v_u64 (0x3fe6900900000000)
#define P(i) v_f64 (__v_log2_data.poly[i])

#define BigBound 0x7ff0000000000000
#define TinyBound 0x0010000000000000

struct entry
{
  float64x2_t invc;
  float64x2_t log2c;
};

static inline struct entry
lookup (uint64x2_t i)
{
  struct entry e;
  e.invc[0] = __v_log2_data.tab[i[0]].invc;
  e.log2c[0] = __v_log2_data.tab[i[0]].log2c;
  e.invc[1] = __v_log2_data.tab[i[1]].invc;
  e.log2c[1] = __v_log2_data.tab[i[1]].log2c;
  return e;
}

VPCS_ATTR
NOINLINE static float64x2_t
specialcase (float64x2_t x, float64x2_t y, uint64x2_t cmp)
{
  return v_call_f64 (log2, x, y, cmp);
}

/* Double-precision vector log2 routine. Implements the same algorithm as vector
   log10, with coefficients and table entries scaled in extended precision.
   The maximum observed error is 2.58 ULP:
   __v_log2(0x1.0b556b093869bp+0) got 0x1.fffb34198d9dap-5
				 want 0x1.fffb34198d9ddp-5.  */
VPCS_ATTR
float64x2_t V_NAME_D1 (log2) (float64x2_t x)
{
  uint64x2_t ix = vreinterpretq_u64_f64 (x);
  uint64x2_t special = ix - TinyBound >= BigBound - TinyBound;

  /* x = 2^k z; where z is in range [OFF,2*OFF) and exact.
     The range is split into N subintervals.
     The ith subinterval contains z and c is near its center.  */
  uint64x2_t tmp = ix - OFF;
  uint64x2_t i = (tmp >> (52 - V_LOG2_TABLE_BITS)) % N;
  int64x2_t k = vreinterpretq_s64_u64 (tmp) >> 52; /* arithmetic shift.  */
  uint64x2_t iz = ix - (tmp & v_u64 (0xfffULL << 52));
  float64x2_t z = vreinterpretq_f64_u64 (iz);
  struct entry e = lookup (i);

  /* log2(x) = log1p(z/c-1)/log(2) + log2(c) + k.  */

  float64x2_t r = vfmaq_f64 (v_f64 (-1.0), z, e.invc);
  float64x2_t kd = vcvtq_f64_s64 (k);
  float64x2_t w = vfmaq_f64 (e.log2c, r, InvLn2);

  float64x2_t r2 = r * r;
  float64x2_t p_23 = vfmaq_f64 (P (2), P (3), r);
  float64x2_t p_01 = vfmaq_f64 (P (0), P (1), r);
  float64x2_t y = vfmaq_f64 (p_23, P (4), r2);
  y = vfmaq_f64 (p_01, r2, y);
  y = vfmaq_f64 (kd + w, r2, y);

  if (unlikely (v_any_u64 (special)))
    return specialcase (x, y, special);
  return y;
}

PL_SIG (V, D, 1, log2, 0.01, 11.1)
PL_TEST_ULP (V_NAME_D1 (log2), 2.09)
PL_TEST_EXPECT_FENV_ALWAYS (V_NAME_D1 (log2))
PL_TEST_INTERVAL (V_NAME_D1 (log2), -0.0, -0x1p126, 100)
PL_TEST_INTERVAL (V_NAME_D1 (log2), 0x1p-149, 0x1p-126, 4000)
PL_TEST_INTERVAL (V_NAME_D1 (log2), 0x1p-126, 0x1p-23, 50000)
PL_TEST_INTERVAL (V_NAME_D1 (log2), 0x1p-23, 1.0, 50000)
PL_TEST_INTERVAL (V_NAME_D1 (log2), 1.0, 100, 50000)
PL_TEST_INTERVAL (V_NAME_D1 (log2), 100, inf, 50000)
