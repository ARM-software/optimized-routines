/*
 *  Single-precision math error handling.
 *
 *  Copyright (C) 2017, ARM Limited, All Rights Reserved
 *  SPDX-License-Identifier: Apache-2.0
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may
 *  not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  This file is part of the Optimized Routines project
 */

#include "math_config.h"

#if WANT_ERRNO
#include <errno.h>
/* NOINLINE reduces code size and avoids making math functions non-leaf
   when the error handling is inlined.  */
NOINLINE static float
with_errnof (float y, int e)
{
  errno = e;
  return y;
}
#else
#define with_errnof(x, e) (x)
#endif

/* NOINLINE prevents fenv semantics breaking optimizations.  */
NOINLINE static float
xflowf (unsigned long sign, float y)
{
  y = (sign ? -y : y) * y;
  return with_errnof (y, ERANGE);
}

HIDDEN float
__math_uflowf (unsigned long sign)
{
  return xflowf (sign, 0x1p-95f);
}

#if WANT_ERRNO_UFLOW
/* Underflows to zero in some non-nearest rounding mode, setting errno
   is valid even if the result is non-zero, but in the subnormal range.  */
HIDDEN float
__math_may_uflowf (unsigned long sign)
{
  return xflowf (sign, 0x1.4p-75f);
}
#endif

HIDDEN float
__math_oflowf (unsigned long sign)
{
  return xflowf (sign, 0x1p97f);
}

HIDDEN float
__math_divzerof (unsigned long sign)
{
  float y = 0;
  return with_errnof ((sign ? -1 : 1) / y, ERANGE);
}

HIDDEN float
__math_invalidf (float x)
{
  float y = (x - x) / (x - x);
  return isnan (x) ? y : with_errnof (y, EDOM);
}
