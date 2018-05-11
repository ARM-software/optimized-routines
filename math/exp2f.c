/*
 * Single-precision 2^x function.
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

#include <math.h>
#include <stdint.h>
#include "math_config.h"

/*
EXP2F_TABLE_BITS = 5
EXP2F_POLY_ORDER = 3

ULP error: 0.502 (nearest rounding.)
Relative error: 1.69 * 2^-34 in [-1/64, 1/64] (before rounding.)
Wrong count: 168353 (all nearest rounding wrong results with fma.)
Non-nearest ULP error: 1 (rounded ULP error)
*/

#define N (1 << EXP2F_TABLE_BITS)
#define T __exp2f_data.tab
#define C __exp2f_data.poly
#define SHIFT __exp2f_data.shift_scaled

static inline uint32_t
top12 (float x)
{
  return asuint (x) >> 20;
}

float
exp2f (float x)
{
  uint32_t abstop;
  uint64_t ki, t;
  /* double_t for better performance on targets with FLT_EVAL_METHOD==2.  */
  double_t kd, xd, z, r, r2, y, s;

  xd = (double_t) x;
  abstop = top12 (x) & 0x7ff;
  if (__builtin_expect (abstop >= top12 (128.0f), 0))
    {
      /* |x| >= 128 or x is nan.  */
      if (asuint (x) == asuint (-INFINITY))
	return 0.0f;
      if (abstop >= top12 (INFINITY))
	return x + x;
      if (x > 0.0f)
	return __math_oflowf (0);
      if (x <= -150.0f)
	return __math_uflowf (0);
#if WANT_ERRNO_UFLOW
      if (x < -149.0f)
	return __math_may_uflowf (0);
#endif
    }

  /* x = k/N + r with r in [-1/(2N), 1/(2N)] and int k.  */
  kd = eval_as_double (xd + SHIFT);
  ki = asuint64 (kd);
  kd -= SHIFT; /* k/N for int k.  */
  r = xd - kd;

  /* exp2(x) = 2^(k/N) * 2^r ~= s * (C0*r^3 + C1*r^2 + C2*r + 1) */
  t = T[ki % N];
  t += ki << (52 - EXP2F_TABLE_BITS);
  s = asdouble (t);
  z = C[0] * r + C[1];
  r2 = r * r;
  y = C[2] * r + 1;
  y = z * r2 + y;
  y = y * s;
  return (float) y;
}
