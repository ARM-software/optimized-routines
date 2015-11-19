/*
 *  k_cos.c
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

/* @(#)k_cos.c 1.3 95/01/18 */
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

/*
 * __kernel_cos( x,  y )
 * kernel cos function on [-pi/4, pi/4], pi/4 ~ 0.785398164
 * Input x is assumed to be bounded by ~pi/4 in magnitude.
 * Input y is the tail of x.
 *
 * Algorithm
 *      1. Since cos(-x) = cos(x), we need only to consider positive x.
 *      2. if x < 2^-27 (hx<0x3e400000 0), return 1 with inexact if x!=0.
 *      3. cos(x) is approximated by a polynomial of degree 14 on
 *         [0,pi/4]
 *                                       4            14
 *              cos(x) ~ 1 - x*x/2 + C1*x + ... + C6*x
 *         where the remez error is
 *
 *      |              2     4     6     8     10    12     14 |     -58
 *      |cos(x)-(1-.5*x +C1*x +C2*x +C3*x +C4*x +C5*x  +C6*x  )| <= 2
 *      |                                                      |
 *
 *                     4     6     8     10    12     14
 *      4. let r = C1*x +C2*x +C3*x +C4*x +C5*x  +C6*x  , then
 *             cos(x) = 1 - x*x/2 + r
 *         since cos(x+y) ~ cos(x) - sin(x)*y
 *                        ~ cos(x) - x*y,
 *         a correction term is necessary in cos(x) and hence
 *              cos(x+y) = 1 - (x*x/2 - (r - x*y))
 *         For better accuracy when x > 0.3, let qx = |x|/4 with
 *         the last 32 bits mask off, and if x > 0.78125, let qx = 0.28125.
 *         Then
 *              cos(x+y) = (1-qx) - ((x*x/2-qx) - (r-x*y)).
 *         Note that 1-qx and (x*x/2-qx) is EXACT here, and the
 *         magnitude of the latter is at least a quarter of x*x/2,
 *         thus, reducing the rounding error in the subtraction.
 */

#include "arm_math.h"
#include "math_private.h"

static const double
one =  0x1p+0, /* 1.00000000000000000000e+00 */
C[] = { 0x1.555555555554cp-5, /* 4.16666666666666019037e-02 */
 -0x1.6c16c16c15177p-10, /* -1.38888888888741095749e-03 */
  0x1.a01a019cb159p-16, /* 2.48015872894767294178e-05 */
 -0x1.27e4f809c52adp-22, /* -2.75573143513906633035e-07 */
  0x1.1ee9ebdb4b1c4p-29, /* 2.08757232129817482790e-09 */
 -0x1.8fae9be8838d4p-37, /* -1.13596475577881948265e-11 */ };

double
ARM__kernel_cos(double x, double y)
{
        double a,hz,z,r,qx;
        int ix;
        ix = __HI(x)&0x7fffffff;        /* ix = |x|'s high word*/
        if(ix<0x3e400000) {                     /* if x < 2**27 */
            if(((int)x)==0) return one;         /* generate inexact */
        }
        z  = x*x;
        r  = z*ARM__kernel_poly(C,6,z);
        r = z*r - x*y;
        z = 0.5 * z;
        if(ix < 0x3FD33333)                     /* if |x| < 0.3 */
            return one - (z - r);
        else {
            if(ix > 0x3fe90000) {               /* x > 0.78125 */
                qx = 0.28125;
            } else {
                __HI(qx) = ix-0x00200000;       /* x/4 */
                __LO(qx) = 0;
            }
            hz = z-qx;
            a  = one-qx;
            return a - (hz - r);
        }
}
