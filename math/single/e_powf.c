/*
 * e_powf.c - single-precision power function
 *
 * Copyright (c) 2009-2015, Arm Limited.
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

#include "arm_math.h"
#include "math_private.h"
#include <math.h>
#include <errno.h>

float
ARM__powf(float x, float y)
{
    float logh, logl;
    float rlogh, rlogl;
    float sign = 1.0f;
    int expadjust = 0;
    unsigned ix, iy;

    ix = fai(x);
    iy = fai(y);

    if (__builtin_expect((ix - 0x00800000) >= (0x7f800000 - 0x00800000) ||
                         ((iy << 1) + 0x02000000) < 0x40000000, 0)) {
        /*
         * The above test rules out, as quickly as I can see how to,
         * all possible inputs except for a normalised positive x
         * being raised to the power of a normalised (and not
         * excessively small) y. That's the fast-path case: if
         * that's what the user wants, we can skip all of the
         * difficult special-case handling.
         *
         * Now we must identify, as efficiently as we can, cases
         * which will return to the fast path with a little tidying
         * up. These are, in order of likelihood and hence of
         * processing:
         *
         *  - a normalised _negative_ x raised to the power of a
         *    non-zero finite y. Having identified this case, we
         *    must categorise y into one of the three categories
         *    'odd integer', 'even integer' and 'non-integer'; for
         *    the last of these we return an error, while for the
         *    other two we rejoin the main code path having rendered
         *    x positive and stored an appropriate sign to append to
         *    the eventual result.
         *
         *  - a _denormal_ x raised to the power of a non-zero
         *    finite y, in which case we multiply it up by a power
         *    of two to renormalise it, store an appropriate
         *    adjustment for its base-2 logarithm, and depending on
         *    the sign of y either return straight to the main code
         *    path or go via the categorisation of y above.
         *
         *  - any of the above kinds of x raised to the power of a
         *    zero, denormal, nearly-denormal or nearly-infinite y,
         *    in which case we must do the checks on x as above but
         *    otherwise the algorithm goes through basically
         *    unchanged. Denormal and very tiny y values get scaled
         *    up to something not in range of accidental underflow
         *    when split into prec-and-a-half format; very large y
         *    values get scaled down by a factor of two to prevent
         *    CLEARBOTTOMHALF's round-up from overflowing them to
         *    infinity. (Of course the _output_ will overflow either
         *    way - the largest y value that can possibly yield a
         *    finite result is well below this range anyway - so
         *    this is a safe change.)
         */
        if (__builtin_expect(((iy << 1) + 0x02000000) >= 0x40000000, 1)) {   /* normalised and sensible y */
            y_ok_check_x:

            if (__builtin_expect((ix - 0x80800000) < (0xff800000 - 0x80800000), 1)) {   /* normal but negative x */
                y_ok_x_negative:

                x = fabsf(x);
                ix = fai(x);

                /*
                 * Determine the parity of y, if it's an integer at
                 * all.
                 */
                {
                    int yexp, yunitsbit;

                    /*
                     * Find the exponent of y.
                     */
                    yexp = (iy >> 23) & 0xFF;
                    /*
                     * Numbers with an exponent smaller than 0x7F
                     * are strictly smaller than 1, and hence must
                     * be fractional.
                     */
                    if (yexp < 0x7F)
                        return MATHERR_POWF_NEGFRAC(x,y);
                    /*
                     * Numbers with an exponent at least 0x97 are by
                     * definition even integers.
                     */
                    if (yexp >= 0x97)
                        goto mainpath;  /* rejoin main code, giving positive result */
                    /*
                     * In between, we must check the mantissa.
                     *
                     * Note that this case includes yexp==0x7f,
                     * which means 1 point something. In this case,
                     * the 'units bit' we're testing is semantically
                     * the lowest bit of the exponent field, not the
                     * leading 1 on the mantissa - but fortunately,
                     * that bit position will just happen to contain
                     * the 1 that we would wish it to, because the
                     * exponent describing that particular case just
                     * happens to be odd.
                     */
                    yunitsbit = 0x96 - yexp;
                    if (iy & ((1 << yunitsbit)-1))
                        return MATHERR_POWF_NEGFRAC(x,y);
                    else if (iy & (1 << yunitsbit))
                        sign = -1.0f;  /* y is odd; result should be negative */
                    goto mainpath;     /* now we can rejoin the main code */
                }
            } else if (__builtin_expect((ix << 1) != 0 && (ix << 1) < 0x01000000, 0)) {   /* denormal x */
                /*
                 * Renormalise x.
                 */
                x *= 0x1p+27F;
                ix = fai(x);
                /*
                 * Set expadjust to compensate for that.
                 */
                expadjust = -27;

                /* Now we need to handle negative x as above. */
                if (ix & 0x80000000)
                    goto y_ok_x_negative;
                else
                    goto mainpath;
            } else if ((ix - 0x00800000) < (0x7f800000 - 0x00800000)) {
                /* normal positive x, back here from denormal-y case below */
                goto mainpath;
            }
        } else if (((iy << 1) + 0x02000000) >= 0x02000000) { /* denormal, nearly-denormal or zero y */
            if (y == 0.0F) {
                 /*
                 * y == 0. Any finite x returns 1 here. (Quiet NaNs
                 * do too, but we handle that below since we don't
                 * mind doing them more slowly.)
                 */
                if ((ix << 1) != 0 && (ix << 1) < 0xFF000000)
                    return 1.0f;
            } else {
                /*
                 * Denormal or very very small y. In this situation
                 * we have to be a bit careful, because when we
                 * break up y into precision-and-a-half later on we
                 * risk working with denormals and triggering
                 * underflow exceptions within this function that
                 * aren't related to the smallness of the output. So
                 * here we convert all such y values into a standard
                 * small-but-not-too-small value which will give the
                 * same output.
                 *
                 * What value should that be? Well, we work in
                 * 16*log2(x) below (equivalently, log to the base
                 * 2^{1/16}). So the maximum magnitude of that for
                 * any finite x is about 2416 (= 16 * (128+23), for
                 * log of the smallest denormal x), i.e. certainly
                 * less than 2^12. If multiplying that by y gives
                 * anything of magnitude less than 2^-32 (and even
                 * that's being generous), the final output will be
                 * indistinguishable from 1. So any value of y with
                 * magnitude less than 2^-(32+12) = 2^-44 is
                 * completely indistinguishable from any other such
                 * value. Hence we got here in the first place by
                 * checking the exponent of y against 64 (i.e. -63,
                 * counting the exponent bias), so we might as well
                 * normalise all tiny y values to the same threshold
                 * of 2^-64.
                 */
                iy = 0x1f800000 | (iy & 0x80000000);   /* keep the sign; that's important */
                y = fhex(iy);
            }
            goto y_ok_check_x;
        } else if (((iy << 1) + 0x02000000) < 0x01000000) { /* y in top finite exponent bracket */
            y = fhex(fai(y) - 0x00800000);   /* scale down by a factor of 2 */
            goto y_ok_check_x;
        }

        /*
         * Having dealt with the above cases, we now know that
         * either x is zero, infinite or NaN, or y is infinite or
         * NaN, or both. We can deal with all of those cases without
         * ever rejoining the main code path.
         */
        if ((unsigned)(((ix & 0x7FFFFFFF) - 0x7f800001) < 0x7fc00000 - 0x7f800001) ||
            (unsigned)(((iy & 0x7FFFFFFF) - 0x7f800001) < 0x7fc00000 - 0x7f800001)) {
            /*
             * At least one signalling NaN. Do a token arithmetic
             * operation on the two operands to provoke an exception
             * and return the appropriate QNaN.
             */
            return FLOAT_INFNAN2(x,y);
        } else if (ix==0x3f800000 || (iy << 1)==0) {
            /*
             * C99 says that 1^anything and anything^0 should both
             * return 1, _even for a NaN_. I modify that slightly to
             * apply only to QNaNs (which doesn't violate C99, since
             * C99 doesn't specify anything about SNaNs at all),
             * because I can't bring myself not to throw an
             * exception on an SNaN since its _entire purpose_ is to
             * throw an exception whenever touched.
             */
            return 1.0f;
        } else
        if (((ix & 0x7FFFFFFF) > 0x7f800000) ||
            ((iy & 0x7FFFFFFF) > 0x7f800000)) {
            /*
             * At least one QNaN. Do a token arithmetic operation on
             * the two operands to get the right one to propagate to
             * the output.
             */
            return FLOAT_INFNAN2(x,y);
        } else if (ix == 0x7f800000) {
            /*
             * x = +infinity. Return +infinity for positive y, +0
             * for negative y, and 1 for zero y.
             */
            if (!(iy << 1))
                return MATHERR_POWF_INF0(x,y);
            else if (iy & 0x80000000)
                return 0.0f;
            else
                return INFINITY;
        } else {
            /*
             * Repeat the parity analysis of y above, returning 1
             * (odd), 2 (even) or 0 (fraction).
             */
            int ypar, yexp, yunitsbit;
            yexp = (iy >> 23) & 0xFF;
            if (yexp < 0x7F)
                ypar = 0;
            else if (yexp >= 0x97)
                ypar = 2;
            else {
                yunitsbit = 0x96 - yexp;
                if (iy & ((1 << yunitsbit)-1))
                    ypar = 0;
                else if (iy & (1 << yunitsbit))
                    ypar = 1;
                else
                    ypar = 2;
            }

            if (ix == 0xff800000) {
                /*
                 * x = -infinity. We return infinity or zero
                 * depending on whether y is positive or negative,
                 * and the sign is negative iff y is an odd integer.
                 * (SGT: I don't like this, but it's what C99
                 * mandates.)
                 */
                if (!(iy & 0x80000000)) {
                    if (ypar == 1)
                        return -INFINITY;
                    else
                        return INFINITY;
                } else {
                    if (ypar == 1)
                        return -0.0f;
                    else
                        return +0.0f;
                }
            } else if (ix == 0) {
                /*
                 * x = +0. We return +0 for all positive y including
                 * infinity; a divide-by-zero-like error for all
                 * negative y including infinity; and an 0^0 error
                 * for zero y.
                 */
                if ((iy << 1) == 0)
                    return MATHERR_POWF_00(x,y);
                else if (iy & 0x80000000)
                    return MATHERR_POWF_0NEGEVEN(x,y);
                else
                    return +0.0f;
            } else if (ix == 0x80000000) {
                /*
                 * x = -0. We return errors in almost all cases (the
                 * exception being positive integer y, in which case
                 * we return a zero of the appropriate sign), but
                 * the errors are almost all different. Gah.
                 */
                if ((iy << 1) == 0)
                    return MATHERR_POWF_00(x,y);
                else if (iy == 0x7f800000)
                    return MATHERR_POWF_NEG0FRAC(x,y);
                else if (iy == 0xff800000)
                    return MATHERR_POWF_0NEG(x,y);
                else if (iy & 0x80000000)
                    return (ypar == 0 ? MATHERR_POWF_0NEG(x,y) :
                            ypar == 1 ? MATHERR_POWF_0NEGODD(x,y) :
                            /* ypar == 2 ? */ MATHERR_POWF_0NEGEVEN(x,y));
                else
                    return (ypar == 0 ? MATHERR_POWF_NEG0FRAC(x,y) :
                            ypar == 1 ? -0.0f :
                            /* ypar == 2 ? */ +0.0f);
            } else {
                /*
                 * Now we know y is an infinity of one sign or the
                 * other and x is finite and nonzero. If x == -1 (+1
                 * is already ruled out), we return +1; otherwise
                 * C99 mandates that we return either +0 or +inf,
                 * the former iff exactly one of |x| < 1 and y<0 is
                 * true.
                 */
                if (ix == 0xbf800000) {
                    return +1.0f;
                } else if (!((ix << 1) < 0x7f000000) ^ !(iy & 0x80000000)) {
                    return +0.0f;
                }
                else {
                    return INFINITY;
                }
            }
        }
    }

    mainpath:

#define PHMULTIPLY(rh,rl, xh,xl, yh,yl) do { \
    float tmph, tmpl; \
    tmph = (xh) * (yh); \
    tmpl = (xh) * (yl) + (xl) * ((yh)+(yl)); \
/* printf("PHMULTIPLY: tmp=%08x+%08x\n", fai(tmph), fai(tmpl)); */ \
    (rh) = CLEARBOTTOMHALF(tmph + tmpl); \
    (rl) = tmpl + (tmph - (rh)); \
} while (0)

/*
 * Same as the PHMULTIPLY macro above, but bounds the absolute value
 * of rh+rl. In multiplications uncontrolled enough that rh can go
 * infinite, we can get an IVO exception from the subtraction tmph -
 * rh, so we should spot that case in advance and avoid it.
 */
#define PHMULTIPLY_SATURATE(rh,rl, xh,xl, yh,yl, bound) do {            \
        float tmph, tmpl;                                               \
        tmph = (xh) * (yh);                                             \
        if (fabsf(tmph) > (bound)) {                                  \
            (rh) = copysignf((bound),(tmph));                           \
            (rl) = 0.0f;                                                \
        } else {                                                        \
            tmpl = (xh) * (yl) + (xl) * ((yh)+(yl));                    \
            (rh) = CLEARBOTTOMHALF(tmph + tmpl);                        \
            (rl) = tmpl + (tmph - (rh));                                \
        }                                                               \
    } while (0)

    /*
     * Determine log2 of x to relative prec-and-a-half, as logh +
     * logl.
     *
     * Well, we're actually computing 16*log2(x), so that it's the
     * right size for the subsequently fiddly messing with powers of
     * 2^(1/16) in the exp step at the end.
     */
    if (__builtin_expect((ix - 0x3f7ff000) <= (0x3f801000 - 0x3f7ff000), 0)) {
        /*
         * For x this close to 1, we write x = 1 + t and then
         * compute t - t^2/2 + t^3/3 - t^4/4; and the neat bit is
         * that t itself, being the bottom half of an input
         * mantissa, is in half-precision already, so the output is
         * naturally in canonical prec-and-a-half form.
         */
        float t = x - 1.0;
        float lnh, lnl;
        /*
         * Compute natural log of x in prec-and-a-half.
         */
        lnh = t;
        lnl = - (t * t) * ((1.0f/2.0f) - t * ((1.0f/3.0f) - t * (1.0f/4.0f)));

        /*
         * Now we must scale by 16/log(2), still in prec-and-a-half,
         * to turn this from natural log(x) into 16*log2(x).
         */
        PHMULTIPLY(logh, logl, lnh, lnl, 0x1.716p+4F, -0x1.7135a8p-9F);
    } else {
        /*
         * For all other x, we start by normalising to [1,2), and
         * then dividing that further into subintervals. For each
         * subinterval we pick a number a in that interval, compute
         * s = (x-a)/(x+a) in precision-and-a-half, and then find
         * the log base 2 of (1+s)/(1-s), still in precision-and-a-
         * half.
         *
         * Why would we do anything so silly? For two main reasons.
         *
         * Firstly, if s = (x-a)/(x+a), then a bit of algebra tells
         * us that x = a * (1+s)/(1-s); so once we've got
         * log2((1+s)/(1-s)), we need only add on log2(a) and then
         * we've got log2(x). So this lets us treat all our
         * subintervals in essentially the same way, rather than
         * requiring a separate approximation for each one; the only
         * correction factor we need is to store a table of the
         * base-2 logs of all our values of a.
         *
         * Secondly, log2((1+s)/(1-s)) is a nice thing to compute,
         * once we've got s. Switching to natural logarithms for the
         * moment (it's only a scaling factor to sort that out at
         * the end), we write it as the difference of two logs:
         *
         *   log((1+s)/(1-s)) = log(1+s) - log(1-s)
         *
         * Now recall that Taylor series expansion gives us
         *
         *   log(1+s) = s - s^2/2 + s^3/3 - ...
         *
         * and therefore we also have
         *
         *   log(1-s) = -s - s^2/2 - s^3/3 - ...
         *
         * These series are exactly the same except that the odd
         * terms (s, s^3 etc) have flipped signs; so subtracting the
         * latter from the former gives us
         *
         *   log(1+s) - log(1-s) = 2s + 2s^3/3 + 2s^5/5 + ...
         *
         * which requires only half as many terms to be computed
         * before the powers of s get too small to see. Then, of
         * course, we have to scale the result by 1/log(2) to
         * convert natural logs into logs base 2.
         *
         * To compute the above series in precision-and-a-half, we
         * first extract a factor of 2s (which we can multiply back
         * in later) so that we're computing 1 + s^2/3 + s^4/5 + ...
         * and then observe that if s starts off small enough to
         * make s^2/3 at most 2^-12, we need only compute the first
         * couple of terms in laborious prec-and-a-half, and can
         * delegate everything after that to a simple polynomial
         * approximation whose error will end up at the bottom of
         * the low word of the result.
         *
         * How many subintervals does that mean we need?
         *
         * To go back to s = (x-a)/(x+a). Let x = a + e, for some
         * positive e. Then |s| = |e| / |2a+e| <= |e/2a|. So suppose
         * we have n subintervals of equal width covering the space
         * from 1 to 2. If a is at the centre of each interval, then
         * we have e at most 1/2n and a can equal any of 1, 1+1/n,
         * 1+2/n, ... 1+(n-1)/n. In that case, clearly the largest
         * value of |e/2a| is given by the largest e (i.e. 1/2n) and
         * the smallest a (i.e. 1); so |s| <= 1/4n. Hence, when we
         * know how big we're prepared to let s be, we simply make
         * sure 1/4n is at most that.
         *
         * And if we want s^2/3 to be at most 2^-12, then that means
         * s^2 is at most 3*2^-12, so that s is at most sqrt(3)*2^-6
         * = 0.02706. To get 1/4n smaller than that, we need to have
         * n>=9.23; so we'll set n=16 (for ease of bit-twiddling),
         * and then s is at most 1/64.
         */
        int n, i;
        float a, ax, sh, sl, lsh, lsl;

        /*
         * Let ax be x normalised to a single exponent range.
         * However, the exponent range in question is not a simple
         * one like [1,2). What we do is to round up the top four
         * bits of the mantissa, so that the top 1/32 of each
         * natural exponent range rounds up to the next one and is
         * treated as a displacement from the lowest a in that
         * range.
         *
         * So this piece of bit-twiddling gets us our input exponent
         * and our subinterval index.
         */
        n = (ix + 0x00040000) >> 19;
        i = n & 15;
        n = ((n >> 4) & 0xFF) - 0x7F;
        ax = fhex(ix - (n << 23));
        n += expadjust;

        /*
         * Compute the subinterval centre a.
         */
        a = 1.0f + i * (1.0f/16.0f);

        /*
         * Compute s = (ax-a)/(ax+a), in precision-and-a-half.
         */
        {
            float u, vh, vl, vapprox, rvapprox;

            u = ax - a;                /* exact numerator */
            vapprox = ax + a;          /* approximate denominator */
            vh = CLEARBOTTOMHALF(vapprox);
            vl = (a - vh) + ax;        /* vh+vl is exact denominator */
            rvapprox = 1.0f/vapprox;   /* approximate reciprocal of denominator */

            sh = CLEARBOTTOMHALF(u * rvapprox);
            sl = ((u - sh*vh) - sh*vl) * rvapprox;
        }

        /*
         * Now compute log2(1+s) - log2(1-s). We do this in several
         * steps.
         *
         * By polynomial approximation, we compute
         *
         *        log(1+s) - log(1-s)
         *    p = ------------------- - 1
         *                2s
         *
         * in single precision only, using a single-precision
         * approximation to s. This polynomial has s^2 as its
         * lowest-order term, so we expect the result to be in
         * [0,2^-12).
         *
         * Then we form a prec-and-a-half number out of 1 and p,
         * which is therefore equal to (log(1+s) - log(1-s))/(2s).
         *
         * Finally, we do two prec-and-a-half multiplications: one
         * by s itself, and one by the constant 32/log(2).
         */
        {
            float s = sh + sl;
            float s2 = s*s;
            /*
             * p is actually a polynomial in s^2, with the first
             * term constrained to zero. In other words, treated on
             * its own terms, we're computing p(s^2) such that p(x)
             * is an approximation to the sum of the series 1/3 +
             * x/5 + x^2/7 + ..., valid on the range [0, 1/40^2].
             */
            float p = s2 * (0.33333332920177422f + s2 * 0.20008275183621479f);
            float th, tl;

            PHMULTIPLY(th,tl, 1.0f,p, sh,sl);
            PHMULTIPLY(lsh,lsl, th,tl, 0x1.716p+5F,-0x1.7135a8p-8F);
        }

        /*
         * And our final answer for 16*log2(x) is equal to 16n (from
         * the exponent), plus lsh+lsl (the result of the above
         * computation), plus 16*log2(a) which we must look up in a
         * table.
         */
        {
            struct f2 { float h, l; };
            static const struct f2 table[16] = {
                /*
                 * When constructing this table, we have to be sure
                 * that we produce the same values of a which will
                 * be produced by the computation above. Ideally, I
                 * would tell Perl to actually do its _arithmetic_
                 * in single precision here; but I don't know a way
                 * to do that, so instead I just scrupulously
                 * convert every intermediate value to and from SP.
                 */
                // perl -e 'for ($i=0; $i<16; $i++) { $v = unpack "f", pack "f", 1/16.0; $a = unpack "f", pack "f", $i * $v; $a = unpack "f", pack "f", $a+1.0; $l = 16*log($a)/log(2); $top = unpack "f", pack "f", int($l*256.0+0.5)/256.0; $bot = unpack "f", pack "f", $l - $top; printf "{0f_%08X,0f_%08X}, ", unpack "VV", pack "ff", $top, $bot; } print "\n"' | fold -s -w56 | sed 's/^/                /'
                {0x0p+0F,0x0p+0F}, {0x1.66p+0F,0x1.fb7d64p-11F},
                {0x1.5cp+1F,0x1.a39fbep-15F}, {0x1.fcp+1F,-0x1.f4a37ep-10F},
                {0x1.49cp+2F,-0x1.87b432p-10F}, {0x1.91cp+2F,-0x1.15db84p-12F},
                {0x1.d68p+2F,-0x1.583f9ap-11F}, {0x1.0c2p+3F,-0x1.f5fe54p-10F},
                {0x1.2b8p+3F,0x1.a39fbep-16F}, {0x1.49ap+3F,0x1.e12f34p-11F},
                {0x1.66ap+3F,0x1.1c8f12p-18F}, {0x1.828p+3F,0x1.3ab7cep-14F},
                {0x1.9d6p+3F,-0x1.30158p-12F}, {0x1.b74p+3F,0x1.291eaap-10F},
                {0x1.d06p+3F,-0x1.8125b4p-10F}, {0x1.e88p+3F,0x1.8d66c4p-10F},
            };
            float lah = table[i].h, lal = table[i].l;
            float fn = 16*n;
            logh = CLEARBOTTOMHALF(lsl + lal + lsh + lah + fn);
            logl = lsl - ((((logh - fn) - lah) - lsh) - lal);
        }
    }

    /*
     * Now we have 16*log2(x), multiply it by y in prec-and-a-half.
     */
    {
        float yh, yl;
        int savedexcepts;

        yh = CLEARBOTTOMHALF(y);
        yl = y - yh;

        /* This multiplication could become infinite, so to avoid IVO
         * in PHMULTIPLY we bound the output at 4096, which is big
         * enough to allow any non-overflowing case through
         * unmodified. Also, we must mask out the OVF exception, which
         * we won't want left in the FP status word in the case where
         * rlogh becomes huge and _negative_ (since that will be an
         * underflow from the perspective of powf's return value, not
         * an overflow). */
        savedexcepts = __ieee_status(0,0) & (FE_IEEE_OVERFLOW | FE_IEEE_UNDERFLOW);
        PHMULTIPLY_SATURATE(rlogh, rlogl, logh, logl, yh, yl, 4096.0f);
        __ieee_status(FE_IEEE_OVERFLOW | FE_IEEE_UNDERFLOW, savedexcepts);
    }

    /*
     * And raise 2 to the power of whatever that gave. Again, this
     * is done in three parts: the fractional part of our input is
     * fed through a polynomial approximation, all but the bottom
     * four bits of the integer part go straight into the exponent,
     * and the bottom four bits of the integer part index into a
     * lookup table of powers of 2^(1/16) in prec-and-a-half.
     */
    {
        float rlog = rlogh + rlogl;
        int i16 = (rlog + (rlog < 0 ? -0.5f : +0.5f));
        float rlogi = i16 >> 4;

        float x = rlogl + (rlogh - i16);

        static const float powersof2to1over16top[16] = { 0x1p+0F, 0x1.0b4p+0F, 0x1.172p+0F, 0x1.238p+0F, 0x1.306p+0F, 0x1.3dep+0F, 0x1.4bep+0F, 0x1.5aap+0F, 0x1.6ap+0F, 0x1.7ap+0F, 0x1.8acp+0F, 0x1.9c4p+0F, 0x1.ae8p+0F, 0x1.c18p+0F, 0x1.d58p+0F, 0x1.ea4p+0F };
        static const float powersof2to1over16bot[16] = { 0x0p+0F, 0x1.586cfap-12F, 0x1.7078fap-13F, 0x1.e9b9d6p-14F, 0x1.fc1464p-13F, 0x1.4c9824p-13F, 0x1.dad536p-12F, 0x1.07dd48p-12F, 0x1.3cccfep-13F, 0x1.1473ecp-12F, 0x1.ca8456p-13F, 0x1.230548p-13F, 0x1.3f32b6p-13F, 0x1.9bdd86p-12F, 0x1.8dcfbap-16F, 0x1.5f454ap-13F };
        static const float powersof2to1over16all[16] = { 0x1p+0F, 0x1.0b5586p+0F, 0x1.172b84p+0F, 0x1.2387a6p+0F, 0x1.306fep+0F, 0x1.3dea64p+0F, 0x1.4bfdaep+0F, 0x1.5ab07ep+0F, 0x1.6a09e6p+0F, 0x1.7a1148p+0F, 0x1.8ace54p+0F, 0x1.9c4918p+0F, 0x1.ae89fap+0F, 0x1.c199bep+0F, 0x1.d5818ep+0F, 0x1.ea4afap+0F };
        /*
         * Coefficients generated using the command

./auxiliary/remez.jl --suffix=f -- '-1/BigFloat(2)' '+1/BigFloat(2)' 2 0 'expm1(x*log(BigFloat(2))/16)/x'

         */
        float p = x * (
            4.332169876512769231967668743345473181486157887703125512683507537369503902991722e-02f+x*(9.384123108485637159805511308039285411735300871134684682779057580789341719567367e-04f+x*(1.355120515540562256928614563584948866224035897564701496826514330445829352922309e-05f))
            );
        int index = (i16 & 15);
        p = powersof2to1over16top[index] + (powersof2to1over16bot[index] + powersof2to1over16all[index]*p);

        if (
            fabsf(rlogi) < 126.0f
            ) {
            return sign * p * fhex((unsigned)((127.0f+rlogi) * 8388608.0f));
        } else if (
                   fabsf(rlogi) < 192.0f
                   ) {
            int i = rlogi;
            float ret;

            ret = sign * p *
                fhex((unsigned)((0x7f+i/2) * 8388608)) *
                fhex((unsigned)((0x7f+i-i/2) * 8388608));

            if ((fai(ret) << 1) == 0xFF000000)
                return MATHERR_POWF_OFL(x, y, sign);
            else if ((fai(ret) << 1) == 0)
                return MATHERR_POWF_UFL(x, y, sign);
            else
                return FLOAT_CHECKDENORM(ret);
        } else {
            if (rlogi < 0)
                return MATHERR_POWF_UFL(x, y, sign);
            else
                return MATHERR_POWF_OFL(x, y, sign);
        }
    }
}
