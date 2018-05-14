/*
 * rredf.h - trigonometric range reduction function written new for RVCT 4.1
 *
 * Copyright (c) 2009, Arm Limited.
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

/*
 * This header file defines an inline function which all three of
 * the single-precision trig functions (sinf, cosf, tanf) should use
 * to perform range reduction. The inline function handles the
 * quickest and most common cases inline, before handing off to an
 * out-of-line function defined in rredf.c for everything else. Thus
 * a reasonable compromise is struck between speed and space. (I
 * hope.) In particular, this approach avoids a function call
 * overhead in the common case.
 */

#ifndef _included_rredf_h
#define _included_rredf_h

#include "math_private.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern float ARM__mathlib_rredf2(float x, int *q, unsigned k);

/*
 * Semantics of the function:
 *  - x is the single-precision input value provided by the user
 *  - the return value is in the range [-pi/4,pi/4], and is equal
 *    (within reasonable accuracy bounds) to x minus n*pi/2 for some
 *    integer n. (FIXME: perhaps some slippage on the output
 *    interval is acceptable, requiring more range from the
 *    following polynomial approximations but permitting more
 *    approximate rred decisions?)
 *  - *q is set to a positive value whose low two bits match those
 *    of n. Alternatively, it comes back as -1 indicating that the
 *    input value was trivial in some way (infinity, NaN, or so
 *    small that we can safely return sin(x)=tan(x)=x,cos(x)=1).
 */
static __inline float ARM__mathlib_rredf(float x, int *q)
{
    /*
     * First, extract the bit pattern of x as an integer, so that we
     * can repeatedly compare things to it without multiple
     * overheads in retrieving comparison results from the VFP.
     */
    unsigned k = fai(x);

    /*
     * Deal immediately with the simplest possible case, in which x
     * is already within the interval [-pi/4,pi/4]. This also
     * identifies the subcase of ludicrously small x.
     */
    if ((k << 1) < (0x3f490fdb << 1)) {
        if ((k << 1) < (0x39800000 << 1))
            *q = -1;
        else
            *q = 0;
        return x;
    }

    /*
     * The next plan is to multiply x by 2/pi and convert to an
     * integer, which gives us n; then we subtract n*pi/2 from x to
     * get our output value.
     *
     * By representing pi/2 in that final step by a prec-and-a-half
     * approximation, we can arrange good accuracy for n strictly
     * less than 2^13 (so that an FP representation of n has twelve
     * zero bits at the bottom). So our threshold for this strategy
     * is 2^13 * pi/2 - pi/4, otherwise known as 8191.75 * pi/2 or
     * 4095.875*pi. (Or, for those perverse people interested in
     * actual numbers rather than multiples of pi/2, about 12867.5.)
     */
    if (__builtin_expect((k & 0x7fffffff) < 0x46490e49, 1)) {
        float nf = 0.636619772367581343f * x;
        /*
         * The difference between that single-precision constant and
         * the real 2/pi is about 2.568e-8. Hence the product nf has a
         * potential error of 2.568e-8|x| even before rounding; since
         * |x| < 4096 pi, that gives us an error bound of about
         * 0.0003305.
         *
         * nf is then rounded to single precision, with a max error of
         * 1/2 ULP, and since nf goes up to just under 8192, half a
         * ULP could be as big as 2^-12 ~= 0.0002441.
         *
         * So by the time we convert nf to an integer, it could be off
         * by that much, causing the wrong integer to be selected, and
         * causing us to return a value a little bit outside the
         * theoretical [-pi/4,+pi/4] output interval.
         *
         * How much outside? Well, we subtract nf*pi/2 from x, so the
         * error bounds above have be be multiplied by pi/2. And if
         * both of the above sources of error suffer their worst cases
         * at once, then the very largest value we could return is
         * obtained by adding that lot to the interval bound pi/4 to
         * get
         *
         *    pi/4 + ((2/pi - 0f_3f22f983)*4096*pi + 2^-12) * pi/2
         *
         * which comes to 0f_3f494b02. (Compare 0f_3f490fdb = pi/4.)
         *
         * So callers of this range reducer should be prepared to
         * handle numbers up to that large.
         */
#ifdef __TARGET_FPU_SOFTVFP
        nf = _frnd(nf);
#else
        if (k & 0x80000000)
            nf = (nf - 8388608.0f) + 8388608.0f;
        else
            nf = (nf + 8388608.0f) - 8388608.0f;   /* round to _nearest_ integer. FIXME: use some sort of frnd in softfp */
#endif
        *q = 3 & (int)nf;
#if 0
        /*
         * FIXME: now I need a bunch of special cases to avoid
         * having to do the full four-word reduction every time.
         * Also, adjust the comment at the top of this section!
         */
        if (__builtin_expect((k & 0x7fffffff) < 0x46490e49, 1))
            return ((x - nf * 0x1.92p+0F) - nf * 0x1.fb4p-12F) - nf * 0x1.4442d2p-24F;
        else
#endif
            return ((x - nf * 0x1.92p+0F) - nf * 0x1.fb4p-12F) - nf * 0x1.444p-24F - nf * 0x1.68c234p-39F;
    }

    /*
     * That's enough to do in-line; if we're still playing, hand off
     * to the out-of-line main range reducer.
     */
    return ARM__mathlib_rredf2(x, q, k);
}

#ifdef __cplusplus
} /* end of extern "C" */
#endif /* __cplusplus */

#endif /* included */

/* end of rredf.h */
