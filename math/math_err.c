/*
 * Double-precision math error handling.
 *
 * Copyright (c) 2018, Arm Limited.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "math_config.h"

#if WANT_ERRNO
#include <errno.h>
/* NOINLINE reduces code size and avoids making math functions non-leaf
   when the error handling is inlined.  */
NOINLINE static double
with_errno (double y, int e)
{
  errno = e;
  return y;
}
#else
#define with_errno(x, e) (x)
#endif

/* NOINLINE prevents fenv semantics breaking optimizations.  */
NOINLINE static double
xflow (uint32_t sign, double y)
{
  y = (sign ? -y : y) * y;
  return with_errno (y, ERANGE);
}

HIDDEN double
__math_uflow (uint32_t sign)
{
  return xflow (sign, 0x1p-767);
}

#if WANT_ERRNO_UFLOW
/* Underflows to zero in some non-nearest rounding mode, setting errno
   is valid even if the result is non-zero, but in the subnormal range.  */
HIDDEN double
__math_may_uflow (uint32_t sign)
{
  return xflow (sign, 0x1.8p-538);
}
#endif

HIDDEN double
__math_oflow (uint32_t sign)
{
  return xflow (sign, 0x1p769);
}

/* NOINLINE prevents fenv semantics breaking optimizations.  */
NOINLINE static double
divzero (double x)
{
  return with_errno (x / 0.0, ERANGE);
}

HIDDEN double
__math_divzero (uint32_t sign)
{
  return divzero (sign ? -1.0 : 1.0);
}

HIDDEN double
__math_invalid (double x)
{
  double y = (x - x) / (x - x);
  return isnan (x) ? y : with_errno (y, EDOM);
}

/* Check result and set errno if necessary.  */

HIDDEN double
__math_check_uflow (double y)
{
  return y == 0.0 ? with_errno (y, ERANGE) : y;
}

HIDDEN double
__math_check_oflow (double y)
{
  return isinf (y) ? with_errno (y, ERANGE) : y;
}
