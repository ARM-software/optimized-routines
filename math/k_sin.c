/*
 *  k_sin.c
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

/* @(#)k_sin.c 1.3 95/01/18 */
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

/* __kernel_sin( x, y, iy)
 * kernel sin function on [-pi/4, pi/4], pi/4 ~ 0.7854
 * Input x is assumed to be bounded by ~pi/4 in magnitude.
 * Input y is the tail of x.
 * Input iy indicates whether y is 0. (if iy=0, y assume to be 0).
 *
 * Algorithm
 *      1. Since sin(-x) = -sin(x), we need only to consider positive x.
 *      2. if x < 2^-27 (hx<0x3e400000 0), return x with inexact if x!=0.
 *      3. sin(x) is approximated by a polynomial of degree 13 on
 *         [0,pi/4]
 *                               3            13
 *              sin(x) ~ x + S1*x + ... + S6*x
 *         where
 *
 *      |sin(x)         2     4     6     8     10     12  |     -58
 *      |----- - (1+S1*x +S2*x +S3*x +S4*x +S5*x  +S6*x   )| <= 2
 *      |  x                                               |
 *
 *      4. sin(x+y) = sin(x) + sin'(x')*y
 *                  ~ sin(x) + (1-x*x/2)*y
 *         For better accuracy, let
 *                   3      2      2      2      2
 *              r = x *(S2+x *(S3+x *(S4+x *(S5+x *S6))))
 *         then                   3    2
 *              sin(x) = x + (S1*x + (x *(r-y/2)+y))
 */

#include "arm_math.h"
#include "math_private.h"
#include <math.h>

static const double
  half =  0x1p-1, /* 5.00000000000000000000e-01 */
  S1 = -0x1.5555555555549p-3, /* -1.66666666666666324348e-01 */
  S[] = { 0x1.111111110f8a6p-7, /* 8.33333333332248946124e-03 */
          -0x1.a01a019c161d5p-13, /* -1.98412698298579493134e-04 */
          0x1.71de357b1fe7dp-19, /* 2.75573137070700676789e-06 */
          -0x1.ae5e68a2b9cebp-26, /* -2.50507602534068634195e-08 */
          0x1.5d93a5acfd57cp-33, /* 1.58969099521155010221e-10 */ };

double
ARM__kernel_sin(double x, double y, int iy)
{
  double z,r,v;
  int ix;
  ix = __HI(x)&0x7fffffff;        /* high word of x */
  if(ix<0x3e400000)                       /* |x| < 2**-27 */
    return DOUBLE_CHECKDENORM(x);
  z       =  x*x;
  v       =  z*x;
  r       =  ARM__kernel_poly(S,5,z);
  if(iy==0) return x+v*(S1+z*r);
  else      return x-((z*(half*y-v*r)-y)-v*S1);
}
