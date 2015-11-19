/*
 *  k_tan.c
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

/* @(#)k_tan.c 1.3 95/01/18 */
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

/* __kernel_tan( x, y, k )
 * kernel tan function on [-pi/4, pi/4], pi/4 ~ 0.7854
 * Input x is assumed to be bounded by ~pi/4 in magnitude.
 * Input y is the tail of x.
 * Input k indicates whether tan (if k=1) or
 * -1/tan (if k= -1) is returned.
 *
 * Algorithm
 *      1. Since tan(-x) = -tan(x), we need only to consider positive x.
 *      2. if x < 2^-28 (hx<0x3e300000 0), return x with inexact if x!=0.
 *      3. tan(x) is approximated by a odd polynomial of degree 27 on
 *         [0,0.67434]
 *                               3             27
 *              tan(x) ~ x + T1*x + ... + T13*x
 *         where
 *
 *              |tan(x)         2     4            26   |     -59.2
 *              |----- - (1+T1*x +T2*x +.... +T13*x    )| <= 2
 *              |  x                                    |
 *
 *         Note: tan(x+y) = tan(x) + tan'(x)*y
 *                        ~ tan(x) + (1+x*x)*y
 *         Therefore, for better accuracy in computing tan(x+y), let
 *                   3      2      2       2       2
 *              r = x *(T2+x *(T3+x *(...+x *(T12+x *T13))))
 *         then
 *                                  3    2
 *              tan(x+y) = x + (T1*x + (x *(r+y)+y))
 *
 *      4. For x in [0.67434,pi/4],  let y = pi/4 - x, then
 *              tan(x) = tan(pi/4-y) = (1-tan(y))/(1+tan(y))
 *                     = 1 - 2*(tan(y) - (tan(y)^2)/(1+tan(y)))
 */

#include <math.h>
#include "math_private.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
#ifdef __STDC__
static const double
#else
static double
#endif
one   =  0x1p+0, /* 1.00000000000000000000e+00 */
pio4  =  0x1.921fb54442d18p-1, /* 7.85398163397448278999e-01 */
pio4lo=  0x1.1a62633145c07p-55, /* 3.06161699786838301793e-17 */
T0    =  0x1.5555555555563p-2, /* 3.33333333333334091986e-01 */
Todd[] = {
  0x1.111111110fe7ap-3, /* 1.33333333333201242699e-01 */
  0x1.664f48406d637p-6, /* 2.18694882948595424599e-02 */
  0x1.d6d22c9560328p-9, /* 3.59207910759131235356e-03 */
  0x1.344d8f2f26501p-11, /* 5.88041240820264096874e-04 */
  0x1.47e88a03792a6p-14, /* 7.81794442939557092300e-05 */
 -0x1.375cbdb605373p-16, /* -1.85586374855275456654e-05 */
},
Teven[] =  {
  0x1.ba1ba1bb341fep-5, /* 5.39682539762260521377e-02 */
  0x1.226e3e96e8493p-7, /* 8.86323982359930005737e-03 */
  0x1.7dbc8fee08315p-10, /* 1.45620945432529025516e-03 */
  0x1.026f71a8d1068p-12, /* 2.46463134818469906812e-04 */
  0x1.2b80f32f0a7e9p-14, /* 7.14072491382608190305e-05 */
  0x1.b2a7074bf7ad4p-16, /* 2.59073051863633712884e-05 */
};


double ARM__kernel_tan(double x, double y, int iy)
{
        double z,r,v,w,s;
        int ix,hx;
        hx = __HI(x);   /* high word of x */
        ix = hx&0x7fffffff;     /* high word of |x| */
        if(ix<0x3e300000) {                      /* x < 2**-28 */
            if((int)x==0) {                    /* generate inexact */
                if(((ix|__LO(x))|(iy+1))==0) return one/fabs(x);
                else return (iy==1)? DOUBLE_CHECKDENORM(x): -one/x;
            }
        }
        if(ix>=0x3FE59428) {                    /* |x|>=0.6744 */
            if(hx<0) {x = -x; y = -y;}
            z = pio4-x;
            w = pio4lo-y;
            x = z+w; y = 0.0;
        }
        z       =  x*x;
        w       =  z*z;
    /* Break x^5*(T[1]+x^2*T[2]+...) into
     *    x^5(T[1]+x^4*T[3]+...+x^20*T[11]) +
     *    x^5(x^2*(T[2]+x^4*T[4]+...+x^22*[T12]))
     */
        r = ARM__kernel_poly(Todd, 6, w);
        v = z*ARM__kernel_poly(Teven, 6, w);
        s = z*x;
        r = y + z*(s*(r+v)+y);
        r += T0*s;
        w = x+r;
        if(ix>=0x3FE59428) {
            v = (double)iy;
            return (double)(1-((hx>>30)&2))*(v-2.0*(x-(w*w/(w+v)-r)));
        }
        if(iy==1) return w;
        else {          /* if allow error up to 2 ulp,
                           simply return -1.0/(x+r) here */
     /*  compute -1.0/(x+r) accurately */
            double a,t;
            z  = w;
            __LO(z) = 0;
            v  = r-(z - x);     /* z+v = r+x */
            t = a  = -1.0/w;    /* a = -1.0/w */
            __LO(t) = 0;
            s  = 1.0+t*z;
            return t+a*(s+t*v);
        }
}

#ifdef __cplusplus
} /* end of extern "C" */
#endif /* __cplusplus */

/* end of tan_i.c */
