/*
 * Double-precision vector log2 function.
 *
 * Copyright (c) 2022, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "include/mathlib.h"
#include "v_math.h"
#if V_SUPPORTED

#define InvLn2 v_f64 (0x1.71547652b82fep0)
#define N (1 << V_LOG2_TABLE_BITS)
#define OFF v_u64 (0x3fe6900900000000)
#define P(i) v_f64 (__v_log2_data.poly[i])

struct entry
{
  v_f64_t invc;
  v_f64_t log2c;
};

static inline struct entry
lookup (v_u64_t i)
{
  struct entry e;
#ifdef SCALAR
  e.invc = __v_log2_data.tab[i].invc;
  e.log2c = __v_log2_data.tab[i].log2c;
#else
  e.invc[0] = __v_log2_data.tab[i[0]].invc;
  e.log2c[0] = __v_log2_data.tab[i[0]].log2c;
  e.invc[1] = __v_log2_data.tab[i[1]].invc;
  e.log2c[1] = __v_log2_data.tab[i[1]].log2c;
#endif
  return e;
}

VPCS_ATTR
NOINLINE static v_f64_t
specialcase (v_f64_t x, v_f64_t y, v_u64_t cmp)
{
  return v_call_f64 (log2, x, y, cmp);
}

/* Double-precision vector log2 routine. Implements the same algorithm as vector
   log10, with coefficients and table entries scaled in extended precision.
   The maximum observed error is 2.59 ULP:
   __v_log2(0x1.0b555054a9bd1p+0) got 0x1.fff6977bdced3p-5
				 want 0x1.fff6977bdced6p-5.  */
VPCS_ATTR
v_f64_t V_NAME (log2) (v_f64_t x)
{
  v_u64_t ix = v_as_u64_f64 (x);
  v_u64_t top = ix >> 48;
  v_u64_t special
    = v_cond_u64 (top - v_u64 (0x0010) >= v_u64 (0x7ff0 - 0x0010));

  /* x = 2^k z; where z is in range [OFF,2*OFF) and exact.
     The range is split into N subintervals.
     The ith subinterval contains z and c is near its center.  */
  v_u64_t tmp = ix - OFF;
  v_u64_t i = (tmp >> (52 - V_LOG2_TABLE_BITS)) % N;
  v_s64_t k = v_as_s64_u64 (tmp) >> 52; /* arithmetic shift.  */
  v_u64_t iz = ix - (tmp & v_u64 (0xfffULL << 52));
  v_f64_t z = v_as_f64_u64 (iz);
  struct entry e = lookup (i);

  /* log2(x) = log1p(z/c-1)/log(2) + log2(c) + k.  */

  v_f64_t r = v_fma_f64 (z, e.invc, v_f64 (-1.0));
  v_f64_t kd = v_to_f64_s64 (k);
  v_f64_t w = v_fma_f64 (r, InvLn2, e.log2c);

  v_f64_t r2 = r * r;
  v_f64_t p_23 = v_fma_f64 (P (3), r, P (2));
  v_f64_t p_01 = v_fma_f64 (P (1), r, P (0));
  v_f64_t y = v_fma_f64 (P (4), r2, p_23);
  y = v_fma_f64 (r2, y, p_01);
  y = v_fma_f64 (r2, y, kd + w);

  if (unlikely (v_any_u64 (special)))
    return specialcase (x, y, special);
  return y;
}
VPCS_ALIAS

#endif
