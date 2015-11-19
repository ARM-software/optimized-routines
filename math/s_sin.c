/*
 *  s_sin.c - double precision sine function
 *
 *  Copyright (C) 1998-2015, ARM Limited, All Rights Reserved
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

/* @(#)s_sin.c 1.3 95/01/18 */
/*
 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunSoft, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice
 * is preserved.
 * ====================================================
 */

/* sin(x)
 * Return sine function of x.
 *
 * kernel function:
 *      __kernel_sin            ... sine function on [-pi/4,pi/4]
 *      __kernel_cos            ... cose function on [-pi/4,pi/4]
 *      __ieee754_rem_pio2      ... argument reduction routine
 *
 * Method.
 *      Let S,C and T denote the sin, cos and tan respectively on
 *      [-PI/4, +PI/4]. Reduce the argument x to y1+y2 = x-k*pi/2
 *      in [-pi/4 , +pi/4], and let n = k mod 4.
 *      We have
 *
 *          n        sin(x)      cos(x)        tan(x)
 *     ----------------------------------------------------------
 *          0          S           C             T
 *          1          C          -S            -1/T
 *          2         -S          -C             T
 *          3         -C           S            -1/T
 *     ----------------------------------------------------------
 *
 * Special cases:
 *      Let trig be any of sin, cos, or tan.
 *      trig(+-INF)  is NaN, with signals;
 *      trig(NaN)    is that NaN;
 *
 * Accuracy:
 *      TRIG(x) returns trig(x) nearly rounded
 */


#include "arm_math.h"
#include "math_private.h"
#include <errno.h>

__attribute__((const)) double
ARM__sin(double x)
{
  double y[2],z=0.0;
  int n, ix;

  /* High word of x. */
  ix = __HI(x);

  /* |x| ~< pi/4 */
  ix &= 0x7fffffff;
  if(ix <= 0x3fe921fb) return ARM__kernel_sin(x,z,0);

  /* sin(Inf) */
  else if (ix==0x7ff00000 && !__LO(x))
    return MATHERR_SIN_INF(x);
  /* sin(NaN) */
  else if (ix>=0x7ff00000)
    return DOUBLE_INFNAN(x);

  /* argument reduction needed */
  else {
    n = ARM__ieee754_rem_pio2(x,y);
    switch(n&3) {
    case 0:  return  ARM__kernel_sin(y[0],y[1],1);
    case 1:  return  ARM__kernel_cos(y[0],y[1]);
    case 2:  return -ARM__kernel_sin(y[0],y[1],1);
    default: return -ARM__kernel_cos(y[0],y[1]);
    }
  }
}
