/*
 * Double-precision vector log10(x) function.
 *
 * Copyright (c) 2022, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "v_log10.h"
#include "include/mathlib.h"
#include "v_math.h"
#if V_SUPPORTED

/* Constants used to switch from base e to base 10.  */
#define ivln10 v_f64 (0x1.bcb7b1526e50ep-2)
#define log10_2 v_f64 (0x1.34413509f79ffp-2)

static const f64_t Poly[] = {
  /* computed from log coeffs divided by log(10) in extended precision then
     rounded to double precision.  */
  -0x1.bcb7b1526e506p-3, 0x1.287a7636be1d1p-3, -0x1.bcb7b158af938p-4,
  0x1.63c78734e6d07p-4, -0x1.287461742fee4p-4,
};

#define A0 v_f64 (Poly[0])
#define A1 v_f64 (Poly[1])
#define A2 v_f64 (Poly[2])
#define A3 v_f64 (Poly[3])
#define A4 v_f64 (Poly[4])
#define Ln2 v_f64 (0x1.62e42fefa39efp-1)
#define N (1 << V_LOG10_TABLE_BITS)
#define OFF v_u64 (0x3fe6900900000000)

struct entry
{
  v_f64_t invc;
  v_f64_t log10c;
};

static inline struct entry
lookup (v_u64_t i)
{
  struct entry e;
#ifdef SCALAR
  e.invc = __v_log10_data[i].invc;
  e.log10c = __v_log10_data[i].log10c;
#else
  e.invc[0] = __v_log10_data[i[0]].invc;
  e.log10c[0] = __v_log10_data[i[0]].log10c;
  e.invc[1] = __v_log10_data[i[1]].invc;
  e.log10c[1] = __v_log10_data[i[1]].log10c;
#endif
  return e;
}

VPCS_ATTR
inline static v_f64_t
specialcase (v_f64_t x, v_f64_t y, v_u64_t cmp)
{
  return v_call_f64 (log10, x, y, cmp);
}

/* Our implementation of v_log10 is a slight modification of v_log (1.660ulps).
   Max ULP error: < 2.5 ulp (nearest rounding.)
   Maximum measured at 2.46 ulp for x in [0.96, 0.97]
     __v_log10(0x1.13192407fcb46p+0) got 0x1.fff6be3cae4bbp-6
				    want 0x1.fff6be3cae4b9p-6
     -0.459999 ulp err 1.96.  */
VPCS_ATTR
v_f64_t V_NAME (log10) (v_f64_t x)
{
  v_f64_t z, r, r2, p, y, kd, hi;
  v_u64_t ix, iz, tmp, top, i, cmp;
  v_s64_t k;
  struct entry e;

  ix = v_as_u64_f64 (x);
  top = ix >> 48;
  cmp = v_cond_u64 (top - v_u64 (0x0010) >= v_u64 (0x7ff0 - 0x0010));

  /* x = 2^k z; where z is in range [OFF,2*OFF) and exact.
     The range is split into N subintervals.
     The ith subinterval contains z and c is near its center.  */
  tmp = ix - OFF;
  i = (tmp >> (52 - V_LOG10_TABLE_BITS)) % N;
  k = v_as_s64_u64 (tmp) >> 52; /* arithmetic shift.  */
  iz = ix - (tmp & v_u64 (0xfffULL << 52));
  z = v_as_f64_u64 (iz);
  e = lookup (i);

  /* log10(x) = log1p(z/c-1)/log(10) + log10(c) + k*log10(2).  */
  r = v_fma_f64 (z, e.invc, v_f64 (-1.0));
  kd = v_to_f64_s64 (k);

  /* hi = r / log(10) + log10(c) + k*log10(2).
     Constants in `v_log10_data.c` are computed (in extended precision) as
     e.log10c := e.logc * ivln10.  */
  v_f64_t w = v_fma_f64 (r, ivln10, e.log10c);

  /* y = log10(1+r) + n * log10(2).  */
  hi = v_fma_f64 (kd, log10_2, w);

  /* y = r2*(A0 + r*A1 + r2*(A2 + r*A3 + r2*A4)) + hi.  */
  r2 = r * r;
  y = v_fma_f64 (A3, r, A2);
  p = v_fma_f64 (A1, r, A0);
  y = v_fma_f64 (A4, r2, y);
  y = v_fma_f64 (y, r2, p);
  y = v_fma_f64 (y, r2, hi);

  if (unlikely (v_any_u64 (cmp)))
    return specialcase (x, y, cmp);
  return y;
}
VPCS_ALIAS

#endif
