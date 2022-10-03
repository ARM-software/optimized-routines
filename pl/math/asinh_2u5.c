/*
 * Double-precision asinh(x) function
 *
 * Copyright (c) 2022, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */
#include "math_config.h"

#define AbsMask 0x7fffffffffffffff
#define ExpM26 0x3e50000000000000 /* asuint64(0x1.0p-26).  */
#define One 0x3ff0000000000000	  /* asuint64(1.0).  */
#define Exp511 0x5fe0000000000000 /* asuint64(0x1.0p511).  */
#define Ln2 0x1.62e42fefa39efp-1
#define C(i) __asinh_data.poly[i]

double
optr_aor_log_f64 (double);

static inline double
eval_poly (double z)
{
  /* Evaluate polynomial using Estrin scheme.  */
  double p_01 = fma (z, C (1), C (0));
  double p_23 = fma (z, C (3), C (2));
  double p_45 = fma (z, C (5), C (4));
  double p_67 = fma (z, C (7), C (6));
  double p_89 = fma (z, C (9), C (8));
  double p_ab = fma (z, C (11), C (10));
  double p_cd = fma (z, C (13), C (12));
  double p_ef = fma (z, C (15), C (14));
  double p_gh = fma (z, C (17), C (16));

  double z2 = z * z;
  double p_03 = fma (z2, p_23, p_01);
  double p_47 = fma (z2, p_67, p_45);
  double p_8b = fma (z2, p_ab, p_89);
  double p_cf = fma (z2, p_ef, p_cd);

  double z4 = z2 * z2;
  double p_07 = fma (z4, p_47, p_03);
  double p_8f = fma (z4, p_cf, p_8b);

  double z8 = z4 * z4;
  double p_0f = fma (z8, p_8f, p_07);

  double z16 = z8 * z8;
  return fma (z16, p_gh, p_0f);
}

/* Scalar double-precision asinh implementation. This routine uses different
   approaches on different intervals:

   |x| < 2^-26: Return x. Function is exact in this region.

   |x| < 1: Use custom order-17 polynomial. This is least accurate close to 1.
     The largest observed error in this region is 1.47 ULPs:
     asinh(0x1.fdfcd00cc1e6ap-1) got 0x1.c1d6bf874019bp-1
				want 0x1.c1d6bf874019cp-1.

   |x| < 2^511: Upper bound of this region is close to sqrt(DBL_MAX). Calculate
     the result directly using the definition asinh(x) = ln(x + sqrt(x*x + 1)).
     The largest observed error in this region is 2.03 ULPs:
     asinh(-0x1.00094e0f39574p+0) got -0x1.c3508eb6a681ep-1
				 want -0x1.c3508eb6a682p-1.

   |x| >= 2^511: We cannot square x without overflow at a low
     cost. At very large x, asinh(x) ~= ln(2x). At huge x we cannot
     even double x without overflow, so calculate this as ln(x) +
     ln(2). The largest observed error in this region is 0.98 ULPs at many
     values, for instance:
     asinh(0x1.5255a4cf10319p+975) got 0x1.52652f4cb26cbp+9
				  want 0x1.52652f4cb26ccp+9.  */
double
asinh (double x)
{
  uint64_t ix = asuint64 (x);
  uint64_t ia = ix & AbsMask;
  double ax = asdouble (ia);
  uint64_t sign = ix & ~AbsMask;

  if (ia < ExpM26)
    {
      return x;
    }

  if (ia < One)
    {
      double x2 = x * x;
      double p = eval_poly (x2);
      double y = fma (p, x2 * ax, ax);
      return asdouble (asuint64 (y) | sign);
    }

  if (unlikely (ia >= Exp511))
    {
      return asdouble (asuint64 (optr_aor_log_f64 (ax) + Ln2) | sign);
    }

  return asdouble (asuint64 (optr_aor_log_f64 (ax + sqrt (ax * ax + 1)))
		   | sign);
}
