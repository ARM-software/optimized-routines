/*
 * Double-precision log10(x) function.
 *
 * Copyright (c) 2020-2022, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "math_config.h"
#include <float.h>
#include <math.h>
#include <stdint.h>

/* Polynomial coefficients and lookup tables.  */
#define T __log10_data.tab
#define T2 __log10_data.tab2
#define B __log10_data.poly1
#define A __log10_data.poly
#define Ln2hi __log10_data.ln2hi
#define Ln2lo __log10_data.ln2lo
#define InvLn10 __log10_data.invln10
#define N (1 << LOG10_TABLE_BITS)
#define OFF 0x3fe6000000000000
#define LO asuint64 (1.0 - 0x1p-5)
#define HI asuint64 (1.0 + 0x1.1p-5)

/* Top 16 bits of a double.  */
static inline uint32_t
top16 (double x)
{
  return asuint64 (x) >> 48;
}

/* Fast and low accuracy implementation of log10.
   The implementation is similar to that of math/log, except that:
   - Polynomials are computed for log10(1+r) with r on same intervals as log.
   - Lookup parameters are scaled (at runtime) to switch from base e to base 10.
   Max ULP error: < 1.7 ulp (nearest rounding.)
     with (LOG10_POLY1_ORDER = 10, LOG10_POLY_ORDER = 6, N = 128)
   Maximum measured at 1.655 ulp for x in [0.0746, 0.0747]:
     log10(0x1.ee008434a44a4p-1) got -0x1.fd415bb39db27p-7
				want -0x1.fd415bb39db29p-7
     +0.344511 ulp err 1.15549.  */
double
log10 (double x)
{
  /* double_t for better performance on targets with FLT_EVAL_METHOD==2.  */
  double_t w, z, r, r2, r3, y, invc, logc, kd;
  uint64_t ix, iz, tmp;
  uint32_t top;
  int k, i;

  ix = asuint64 (x);
  top = top16 (x);

  if (unlikely (ix - LO < HI - LO))
    {
      /* Handle close to 1.0 inputs separately.  */
      /* Fix sign of zero with downward rounding when x==1.  */
      if (WANT_ROUNDING && unlikely (ix == asuint64 (1.0)))
	return 0;
      r = x - 1.0;
      r2 = r * r;
      r3 = r * r2;
      /* Worst-case error is around 0.727 ULP.  */
      y = r3
	  * (B[1] + r * B[2] + r2 * B[3]
	     + r3 * (B[4] + r * B[5] + r2 * B[6] + r3 * (B[7] + r * B[8])));
      w = B[0] * r2; /* B[0] == -0.5.  */
      /* Scale by 1/ln(10). Polynomial already contains scaling.  */
      y = (y + w) + r * InvLn10;

      return eval_as_double (y);
    }
  if (unlikely (top - 0x0010 >= 0x7ff0 - 0x0010))
    {
      /* x < 0x1p-1022 or inf or nan.  */
      if (ix * 2 == 0)
	return __math_divzero (1);
      if (ix == asuint64 (INFINITY)) /* log10(inf) == inf.  */
	return x;
      if ((top & 0x8000) || (top & 0x7ff0) == 0x7ff0)
	return __math_invalid (x);
      /* x is subnormal, normalize it.  */
      ix = asuint64 (x * 0x1p52);
      ix -= 52ULL << 52;
    }

  /* x = 2^k z; where z is in range [OFF,2*OFF) and exact.
     The range is split into N subintervals.
     The ith subinterval contains z and c is near its center.  */
  tmp = ix - OFF;
  i = (tmp >> (52 - LOG10_TABLE_BITS)) % N;
  k = (int64_t) tmp >> 52; /* arithmetic shift.  */
  iz = ix - (tmp & 0xfffULL << 52);
  invc = T[i].invc;
  logc = T[i].logc;
  z = asdouble (iz);

  /* log(x) = log1p(z/c-1) + log(c) + k*Ln2.  */
  /* r ~= z/c - 1, |r| < 1/(2*N).  */
#if HAVE_FAST_FMA
  /* rounding error: 0x1p-55/N.  */
  r = fma (z, invc, -1.0);
#else
  /* rounding error: 0x1p-55/N + 0x1p-66.  */
  r = (z - T2[i].chi - T2[i].clo) * invc;
#endif
  kd = (double_t) k;

  /* w = log(c) + k*Ln2hi.  */
  w = kd * Ln2hi + logc;

  /* log10(x) = (w + r)/log(10) + (log10(1+r) - r/log(10)).  */
  r2 = r * r; /* rounding error: 0x1p-54/N^2.  */
  y = r2 * A[0] + r * r2 * (A[1] + r * A[2] + r2 * (A[3] + r * A[4]));

  /* Scale by 1/ln(10). Polynomial already contains scaling.  */
  y = y + ((r + kd * Ln2lo) + w) * InvLn10;

  return eval_as_double (y);
}
#if USE_GLIBC_ABI
strong_alias (log10, __log10_finite)
hidden_alias (log10, __ieee754_log10)
#if LDBL_MANT_DIG == 53
long double
log10l (long double x)
{
  return log10 (x);
}
#endif
#endif
