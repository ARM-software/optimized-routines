/*
 * Extended precision scalar reference functions for C23.
 *
 * Copyright (c) 2023-2025, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "math_config.h"

#ifndef M_PIl
#  define M_PIl 3.141592653589793238462643383279502884l
#endif
#ifndef M_INV_LOG2l
#  define M_INV_LOG2l 0x1.71547652b82fe1777d0ffda0d23a7d11d6aef551cp+0
#endif
#ifndef M_INV_LOG10
#  define M_INV_LOG10 0x1.bcb7b1526e50ep-2
#endif
#ifndef M_INV_LOG10l
#  define M_INV_LOG10l 0x1.bcb7b1526e50e32a6ab7555f5a67b8647dc68c049p-2l
#endif
#ifndef M_LOG2
#  define M_LOG2 0x1.62e42fefa39efp-1
#endif
#ifndef M_LOG2l
#  define M_LOG2l 0x1.62e42fefa39ef35793c7673007e6p-1l
#endif

long double
arm_math_sinpil (long double x)
{
  /* sin(inf) should return nan, as defined by C23.  */
  if (isinf (x))
    return __math_invalid (x);

  long double ax = fabsl (x);

  /* Return 0 for all values above 2^64 to prevent
     overflow when casting to uint64_t.  */
  if (ax >= 0x1p64)
    return x < 0 ? -0.0l : 0.0l;

  /* All integer cases should return 0, with unchanged sign for zero.  */
  if (x == 0.0l)
    return x;
  if (ax == (uint64_t) ax)
    return x < 0 ? -0.0l : 0.0l;

  return sinl (x * M_PIl);
}

long double
arm_math_cospil (long double x)
{
  /* cos(inf) should return nan, as defined by C23.  */
  if (isinf (x))
    return __math_invalid (x);

  long double ax = fabsl (x);

  if (ax >= 0x1p64)
    return 1;

  uint64_t m = (uint64_t) ax;

  /* Integer values of cospi(x) should return +/-1.
    The sign depends on if x is odd or even.  */
  if (m == ax)
    return (m & 1) ? -1 : 1;

  /* Values of Integer + 0.5 should always return 0.  */
  if (ax - 0.5 == m || ax + 0.5 == m)
    return 0;

  return cosl (ax * M_PIl);
}

long double
arm_math_tanpil (long double x)
{
  /* inf and x = n + 0.5 for any integral n should return nan.  */
  if (fabsl (x) >= 0x1p54l)
    {
      if (isinf (x))
	return __math_invalid (x);
      return x < 0 ? -0.0l : 0.0l;
    }

  long double i = roundl (x);
  long double f = x - i;
  int64_t m = (int64_t) i;

  if (x == 0)
    {
      return x;
    }
  else if (x == i)
    {
      if (x < 0)
	{
	  return m & 1 ? 0.0l : -0.0l;
	}
      else
	{
	  return m & 1 ? -0.0l : 0.0l;
	}
    }
  else if (fabsl (f) == 0.5l)
    {
      if (x < 0)
	{
	  return m & 1 ? -1.0l / 0.0l : 1.0l / 0.0l;
	}
      else
	{
	  return m & 1 ? 1.0l / 0.0l : -1.0l / 0.0l;
	}
    }

  return tanl (f * M_PIl);
}

double
arm_math_acospi (double x)
{
  return acos (x) / M_PIl;
}

long double
arm_math_acospil (long double x)
{
  return acosl (x) / M_PIl;
}

double
arm_math_asinpi (double x)
{
  return asin (x) / M_PIl;
}

long double
arm_math_asinpil (long double x)
{
  return asinl (x) / M_PIl;
}

double
arm_math_atanpi (double x)
{
  return atan (x) / M_PIl;
}

long double
arm_math_atanpil (long double x)
{
  return atanl (x) / M_PIl;
}

double
arm_math_atan2pi (double x, double y)
{
  return atan2 (x, y) / M_PIl;
}

long double
arm_math_atan2pil (long double x, long double y)
{
  return atan2l (x, y) / M_PIl;
}

double
arm_math_exp10m1 (double x)
{
  long double xln10 = x * 0x1.26bb1bbb5551582dd4adac5705a6p1l;
  /* exp10 is a GNU extension, so for comptability, use pow.  */
  return (fabsl (x) < 0x1p-55) ? xln10 : powl (10, x) - 1.0l;
}

long double
arm_math_exp10m1l (long double x)
{
  long double xln10 = x * 0x1.26bb1bbb5551582dd4adac5705a6p1l;
  /* exp10 is a GNU extension, so for comptability, use pow.  */
  return (fabsl (x) < 0x1p-55) ? xln10 : powl (10, x) - 1.0l;
}

double
arm_math_exp2m1 (double x)
{
  return (fabs (x) < 0x1p-23) ? x * M_LOG2 : exp2 (x) - 1.0;
}

long double
arm_math_exp2m1l (long double x)
{
  return (fabsl (x) < 0x1p-52l) ? x * M_LOG2l : exp2l (x) - 1.0l;
}

double
arm_math_log2p1 (double x)
{
  return log1p (x) * M_INV_LOG2l;
}

long double
arm_math_log2p1l (long double x)
{
  return (fabsl (x) < 0x1p-52l) ? (long double) x * M_INV_LOG2l
				: (log1pl ((long double) x) / logl (2));
}

double
arm_math_log10p1 (double x)
{
  return log1p (x) * M_INV_LOG10;
}

long double
arm_math_log10p1l (long double x)
{
  return log1pl (x) * M_INV_LOG10l;
}