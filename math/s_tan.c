/*
 *  s_tan.c - double precision tangent function
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

/* @(#)s_tan.c 1.3 95/01/18 */
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

/* tan(x)
 * Return tangent function of x.
 *
 * kernel function:
 *      __kernel_tan            ... tangent function on [-pi/4,pi/4]
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

/*
 * David Seal once wondered about the possibility of tan()
 * overflowing. It's perfectly possible in principle: if you
 * provide an input number x _very_ close to (k+1/2)pi for some
 * integer k, then cos(x) will be denormal and sin(x) will be
 * effectively 1, and hence tan(x) will be just off the top of the
 * representable number range.
 *
 * Fortunately, using Tim Peters' modmin algorithm[1], it's
 * practically feasible to search _all_ the representable double
 * precision numbers and find the ones which are closest to a
 * multiple-and-a-half of pi. The algorithm only works on a range
 * of _equally spaced_ numbers, so of course you need to do a
 * separate search for each possible exponent, but it's a very fast
 * algorithm, 2048 times negligible time still doesn't take very
 * long, and this is still feasible.
 *
 * I (Simon Tatham) actually did this, and it turns out that the
 * minimum negative value of cos(x) is 0x3c214ae7.2e6ba22e.f46
 * (that's expressed in double precision with a few extra bits),
 * and the minimum positive value is 0xbc3f54f5.227a4e83.fbf. The
 * `double' inputs which generate those values are
 * 0x7506ac5b.262ca1ff and 0x416b951f.1572eba5 respectively.
 *
 * Hence, no representable double has a cosine smaller than about
 * 2^-60, and accordingly tan() can never return anything bigger
 * than about 2^60, which doesn't even overflow in _single_
 * precision let alone double. We are safe.
 *
 * [1] A Program for Testing IEEE Decimal-Binary Conversion, Vern
 * Paxson and Prof W. Kahan, available from
 * ftp://ftp.ee.lbl.gov/testbase-report.ps.Z
 */

#include <math.h>
#include <math_private.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

double ARM__tan(double x)
{
    double y[2],z=0.0;
    int n, ix;

    /* High word of x. */
    ix = __HI(x);

    /* |x| ~< pi/4 */
    ix &= 0x7fffffff;
    if(ix <= 0x3fe921fb) return ARM__kernel_tan(x,z,1);

    /* tan(Inf) */
    else if (ix==0x7ff00000 && !__LO(x))
        return MATHERR_TAN_INF(x);
    /* tan(NaN) */
    else if (ix>=0x7ff00000)
        return DOUBLE_INFNAN(x);

    /* argument reduction needed */
    else {
        n = ARM__ieee754_rem_pio2(x,y);
        return ARM__kernel_tan(y[0],y[1],1-((n&1)<<1)); /*   1 -- n even
                                                      -1 -- n odd */
    }
}

#ifdef __cplusplus
} /* end of extern "C" */
#endif /* __cplusplus */

/* end of tan.c */
