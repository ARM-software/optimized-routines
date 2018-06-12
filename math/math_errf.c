/*
 * Single-precision math error handling.
 *
 * Copyright (c) 2017, Arm Limited.
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
NOINLINE static float
with_errnof (float y, int e)
{
  errno = e;
  return y;
}
#else
#define with_errnof(x, e) (x)
#endif

/* NOINLINE reduces code size.  */
NOINLINE static float
xflowf (uint32_t sign, float y)
{
  y = opt_barrier_float (sign ? -y : y) * y;
  return with_errnof (y, ERANGE);
}

HIDDEN float
__math_uflowf (uint32_t sign)
{
  return xflowf (sign, 0x1p-95f);
}

#if WANT_ERRNO_UFLOW
/* Underflows to zero in some non-nearest rounding mode, setting errno
   is valid even if the result is non-zero, but in the subnormal range.  */
HIDDEN float
__math_may_uflowf (uint32_t sign)
{
  return xflowf (sign, 0x1.4p-75f);
}
#endif

HIDDEN float
__math_oflowf (uint32_t sign)
{
  return xflowf (sign, 0x1p97f);
}

HIDDEN float
__math_divzerof (uint32_t sign)
{
  float y = opt_barrier_float (sign ? -1.0f : 1.0f) / 0.0f;
  return with_errnof (y, ERANGE);
}

HIDDEN float
__math_invalidf (float x)
{
  float y = (x - x) / (x - x);
  return isnan (x) ? y : with_errnof (y, EDOM);
}
