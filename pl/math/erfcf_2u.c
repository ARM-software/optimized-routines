/*
 * Single-precision erfc(x) function.
 *
 * Copyright (c) 2019-2022, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "math_config.h"

#define P(i) __erfcf_poly_data.poly[i]

/* Accurate exponential from optimized routines.  */
double
__exp_dd (double x, double xtail);

/* Evaluate order-12 polynomials using pairwise summation and Horner scheme in
   double precision.  */
static inline double
eval_poly_horner_lvl2 (double z, const double *coeff)
{
  double r1, r2, r3, r4, r5, r6, r7, r8;
  double R1, R2, R3, R4;
  double Q1, Q2;
  double z2, z4, z8;
  z2 = z * z;
  r1 = fma (z, coeff[1], coeff[0]);
  r2 = fma (z, coeff[3], coeff[2]);
  z4 = z2 * z2;
  z8 = z4 * z4;
  R1 = fma (z2, r2, r1);
  r3 = fma (z, coeff[5], coeff[4]);
  r4 = fma (z, coeff[7], coeff[6]);
  R2 = fma (z2, r4, r3);
  Q1 = fma (z4, R2, R1);
  r5 = fma (z, coeff[9], coeff[8]);
  r6 = fma (z, coeff[11], coeff[10]);
  R3 = fma (z2, r6, r5);
  r7 = fma (z, coeff[13], coeff[12]);
  r8 = fma (z, coeff[15], coeff[14]);
  R4 = fma (z2, r8, r7);
  Q2 = fma (z4, R4, R3);
  return fma (z8, Q2, Q1);
}

static inline double
eval_exp_mx2 (double x)
{
  return __exp_dd (-(x * x), 0.0);
}

/* Approximation of erfcf for |x| > 4.0.  */
static inline float
approx_erfcf_hi (float x, uint32_t sign, const double *coeff)
{
  if (sign)
    {
      return 2.0f;
    }

  /* Polynomial contribution.  */
  double z = (double) fabs (x);
  float p = (float) eval_poly_horner_lvl2 (z, coeff);
  /* Gaussian contribution.  */
  float e_mx2 = (float) eval_exp_mx2 (z);

  return p * e_mx2;
}

/* Approximation of erfcf for |x| < 4.0.  */
static inline float
approx_erfcf_lo (float x, uint32_t sign, const double *coeff)
{
  /* Polynomial contribution.  */
  double z = (double) fabs (x);
  float p = (float) eval_poly_horner_lvl2 (z, coeff);
  /* Gaussian contribution.  */
  float e_mx2 = (float) eval_exp_mx2 (z);

  if (sign)
    return fmaf (-p, e_mx2, 2.0f);
  else
    return p * e_mx2;
}

/* Top 12 bits of a float (sign and exponent bits).  */
static inline uint32_t
abstop12 (float x)
{
  return (asuint (x) >> 20) & 0x7ff;
}

/* Top 12 bits of a float.  */
static inline uint32_t
top12 (float x)
{
  return asuint (x) >> 20;
}

/* Fast erfcf approximation using polynomial approximation
   multiplied by gaussian.
   Most of the computation is carried out in double precision,
   and is very sensitive to accuracy of polynomial and exp
   evaluation.
   Worst-case error is 1.968ulps, obtained for x = 2.0412941.
   erfcf(0x1.05492p+1) got 0x1.fe10f6p-9 want 0x1.fe10f2p-9 ulp
   err 1.46788.  */
float
erfcf (float x)
{
  /* Get top words and sign.  */
  uint32_t ix = asuint (x); /* We need to compare at most 32 bits.  */
  uint32_t sign = ix >> 31;
  uint32_t ia12 = top12 (x) & 0x7ff;

  /* Handle special cases and small values with a single comparison:
       abstop12(x)-abstop12(small) >= abstop12(INFINITY)-abstop12(small)

     Special cases
       erfcf(nan)=nan, erfcf(+inf)=0 and erfcf(-inf)=2

     Errno
       EDOM does not have to be set in case of erfcf(nan).
       Only ERANGE may be set in case of underflow.

     Small values (|x|<small)
       |x|<0x1.0p-26 => accurate to 0.5 ULP (top12(0x1p-26) = 0x328).  */
  if (unlikely (abstop12 (x) - 0x328 >= (abstop12 (INFINITY) & 0x7f8) - 0x328))
    {
      if (abstop12 (x) >= 0x7f8)
	return (float) (sign << 1) + 1.0f / x; /* Special cases.  */
      else
	return 1.0f - x; /* Small case.  */
    }

  /* Normalized numbers divided in 4 intervals
     with bounds: 2.0, 4.0, 8.0 and 10.0. 10 was chosen as the upper bound for
     the interesting region as it is the smallest value, representable as a
     12-bit integer, for which returning 0 gives <1.5 ULP.  */
  if (ia12 < 0x400)
    {
      return approx_erfcf_lo (x, sign, P (0));
    }
  if (ia12 < 0x408)
    {
      return approx_erfcf_lo (x, sign, P (1));
    }
  if (ia12 < 0x410)
    {
      return approx_erfcf_hi (x, sign, P (2));
    }
  if (ia12 < 0x412)
    {
      return approx_erfcf_hi (x, sign, P (3));
    }
  if (sign)
    {
      return 2.0f;
    }
  return __math_uflowf (0);
}
