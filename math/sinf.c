/*
 * Single-precision sin function.
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

#if WANT_SINGLEPREC
#include "single/s_sinf.c"
#else

#include <math.h>
#include "math_config.h"
#include "sincosf.h"

/* Fast sinf implementation.  Worst-case ULP is 0.56072, maximum relative
   error is 0.5303p-23.  A single-step signed range reduction is used for
   small values.  Large inputs have their range reduced using fast integer
   arithmetic.
*/
float
sinf (float y)
{
  double x = y;
  double s;
  int n;
  sincos_t *p = &sincosf_table[0];

  if (abstop12 (y) < abstop12 (pio4))
    {
      s = x * x;

      if (unlikely (abstop12 (y) < abstop12 (0x1p-12f)))
	{
	  if (unlikely (abstop12 (y) < abstop12 (0x1p-126f)))
	    /* Force underflow for tiny y.  */
	    force_eval_float (s);
	  return y;
	}

      return sinf_poly (x, s, p, 0);
    }
  else if (likely (abstop12 (y) < abstop12 (120.0f)))
    {
      x = reduce_fast (x, p, &n);

      /* Setup the signs for sin and cos.  */
      s = p->sign[n & 3];

      if (n & 2)
	p = &sincosf_table[1];

      return sinf_poly (x * s, x * x, p, n);
    }
  else if (abstop12 (y) < abstop12 (INFINITY))
    {
      uint32_t xi = asuint (y);
      int sign = xi >> 31;

      x = reduce_large (xi, &n);

      /* Setup signs for sin and cos - include original sign.  */
      s = p->sign[(n + sign) & 3];

      if ((n + sign) & 2)
	p = &sincosf_table[1];

      return sinf_poly (x * s, x * x, p, n);
    }
  else
    return __math_invalidf (y);
}

#endif
