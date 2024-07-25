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
sinpil (long double x)
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
cospil (long double x)
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
