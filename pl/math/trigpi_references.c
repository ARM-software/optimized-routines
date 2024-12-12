/*
 * Extended precision scalar reference functions for trigpi.
 *
 * Copyright (c) 2023-2024, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#define _GNU_SOURCE
#include "math_config.h"
#include "mathlib.h"

#ifndef M_PIl
# define M_PIl 3.141592653589793238462643383279502884l
#endif

long double
tanpil (long double x)
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
