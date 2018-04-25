/*
 * Single-precision e^x function.
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

#if WANT_SINGLEPREC
#include "single/e_expf.c"
#else

#include <math.h>
#include <stdint.h>
#include "math_config.h"

/*
EXP2F_TABLE_BITS = 5
EXP2F_POLY_ORDER = 3

ULP error: 0.502 (nearest rounding.)
Relative error: 1.69 * 2^-34 in [-ln2/64, ln2/64] (before rounding.)
Wrong count: 170635 (all nearest rounding wrong results with fma.)
Non-nearest ULP error: 1 (rounded ULP error)
*/

#define N (1 << EXP2F_TABLE_BITS)
#define InvLn2N __exp2f_data.invln2_scaled
#define T __exp2f_data.tab
#define C __exp2f_data.poly_scaled

static inline uint32_t
top12 (float x)
{
  return asuint (x) >> 20;
}

float
expf (float x)
{
  uint32_t abstop;
  uint64_t ki, t;
  /* double_t for better performance on targets with FLT_EVAL_METHOD==2.  */
  double_t kd, xd, z, r, r2, y, s;

  xd = (double_t) x;
  abstop = top12 (x) & 0x7ff;
  if (__builtin_expect (abstop >= top12 (88.0f), 0))
    {
      /* |x| >= 88 or x is nan.  */
      if (asuint (x) == asuint (-INFINITY))
	return 0.0f;
      if (abstop >= top12 (INFINITY))
	return x + x;
      if (x > 0x1.62e42ep6f) /* x > log(0x1p128) ~= 88.72 */
	return __math_oflowf (0);
      if (x < -0x1.9fe368p6f) /* x < log(0x1p-150) ~= -103.97 */
	return __math_uflowf (0);
#if WANT_ERRNO_UFLOW
      if (x < -0x1.9d1d9ep6f) /* x < log(0x1p-149) ~= -103.28 */
	return __math_may_uflowf (0);
#endif
    }

  /* x*N/Ln2 = k + r with r in [-1/2, 1/2] and int k.  */
  z = InvLn2N * xd;

  /* Round and convert z to int, the result is in [-150*N, 128*N] and
     ideally ties-to-even rule is used, otherwise the magnitude of r
     can be bigger which gives larger approximation error.  */
#if TOINT_INTRINSICS
  kd = roundtoint (z);
  ki = converttoint (z);
#elif TOINT_RINT
  kd = rint (z);
  ki = (long) kd;
#elif TOINT_SHIFT
# define SHIFT __exp2f_data.shift
  kd = (double) (z + SHIFT); /* Rounding to double precision is required.  */
  ki = asuint64 (kd);
  kd -= SHIFT;
#endif
  r = z - kd;

  /* exp(x) = 2^(k/N) * 2^(r/N) ~= s * (C0*r^3 + C1*r^2 + C2*r + 1) */
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
#endif
