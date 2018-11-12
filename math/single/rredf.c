/*
 * rredf.c - trigonometric range reduction function
 *
 * Copyright (c) 2009-2018, Arm Limited.
 * SPDX-License-Identifier: MIT
 */

/*
 * This code is intended to be used as the second half of a range
 * reducer whose first half is an inline function defined in
 * rredf.h. Each trig function performs range reduction by invoking
 * that, which handles the quickest and most common cases inline
 * before handing off to this function for everything else. Thus a
 * reasonable compromise is struck between speed and space. (I
 * hope.) In particular, this approach avoids a function call
 * overhead in the common case.
 */

#include "math_private.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*
 * Input values to this function:
 *  - x is the original user input value, unchanged by the
 *    first-tier reducer in the case where it hands over to us.
 *  - q is still the place where the caller expects us to leave the
 *    quadrant code.
 *  - k is the IEEE bit pattern of x (which it would seem a shame to
 *    recompute given that the first-tier reducer already went to
 *    the effort of extracting it from the VFP). FIXME: in softfp,
 *    on the other hand, it's unconscionably wasteful to replicate
 *    this value into a second register and we should change the
 *    prototype!
 */
float __mathlib_rredf2(float x, int *q, unsigned k)
{
    /*
     * First, weed out infinities and NaNs, and deal with them by
     * returning a negative q.
     */
    if ((k << 1) >= 0xFF000000) {
        *q = -1;
        return x;
    }
    /*
     * We do general range reduction by multiplying by 2/pi, and
     * retaining the bottom two bits of the integer part and an
     * initial chunk of the fraction below that. The integer bits
     * are directly output as *q; the fraction is then multiplied
     * back up by pi/2 before returning it.
     *
     * To get this right, we don't have to multiply by the _whole_
     * of 2/pi right from the most significant bit downwards:
     * instead we can discard any bit of 2/pi with a place value
     * high enough that multiplying it by the LSB of x will yield a
     * place value higher than 2. Thus we can bound the required
     * work by a reasonably small constant regardless of the size of
     * x (unlike, for instance, the IEEE remainder operation).
     *
     * At the other end, however, we must take more care: it isn't
     * adequate just to acquire two integer bits and 24 fraction
     * bits of (2/pi)x, because if a lot of those fraction bits are
     * zero then we will suffer significance loss. So we must keep
     * computing fraction bits as far down as 23 bits below the
     * _highest set fraction bit_.
     *
     * The immediate question, therefore, is what the bound on this
     * end of the job will be. In other words: what is the smallest
     * difference between an integer multiple of pi/2 and a
     * representable IEEE single precision number larger than the
     * maximum size handled by rredf.h?
     *
     * The most difficult cases for each exponent can readily be
     * found by Tim Peters's modular minimisation algorithm, and are
     * tabulated in mathlib/tests/directed/rredf.tst. The single
     * worst case is the IEEE single-precision number 0x6F79BE45,
     * whose numerical value is in the region of 7.7*10^28; when
     * reduced mod pi/2, it attains the value 0x30DDEEA9, or about
     * 0.00000000161. The highest set bit of this value is the one
     * with place value 2^-30; so its lowest is 2^-53. Hence, to be
     * sure of having enough fraction bits to output at full single
     * precision, we must be prepared to collect up to 53 bits of
     * fraction in addition to our two bits of integer part.
     *
     * To begin with, this means we must store the value of 2/pi to
     * a precision of 128+53 = 181 bits. That's six 32-bit words.
     * (Hardly a chore, unlike the equivalent problem in double
     * precision!)
     */
    {
        static const unsigned twooverpi[] = {
            /* We start with a zero word, because that takes up less
             * space than the array bounds checking and special-case
             * handling that would have to occur in its absence. */
            0,
            /* 2/pi in hex is 0.a2f9836e... */
            0xa2f9836e, 0x4e441529, 0xfc2757d1,
            0xf534ddc0, 0xdb629599, 0x3c439041,
            /* Again, to avoid array bounds overrun, we store a spare
             * word at the end. And it would be a shame to fill it
             * with zeroes when we could use more bits of 2/pi... */
            0xfe5163ab
        };

        /*
         * Multiprecision multiplication of this nature is more
         * readily done in integers than in VFP, since we can use
         * UMULL (on CPUs that support it) to multiply 32 by 32 bits
         * at a time whereas the VFP would only be able to do 12x12
         * without losing accuracy.
         *
         * So extract the mantissa of the input number as a 32-bit
         * integer.
         */
        unsigned mantissa = 0x80000000 | (k << 8);

        /*
         * Now work out which part of our stored value of 2/pi we're
         * supposed to be multiplying by.
         *
         * Let the IEEE exponent field of x be e. With its bias
         * removed, (e-127) is the index of the set bit at the top
         * of 'mantissa' (i.e. that set bit has real place value
         * 2^(e-127)). So the lowest set bit in 'mantissa', 23 bits
         * further down, must have place value 2^(e-150).
         *
         * We begin taking an interest in the value of 2/pi at the
         * bit which multiplies by _that_ to give something with
         * place value at most 2. In other words, the highest bit of
         * 2/pi we're interested in is the one with place value
         * 2/(2^(e-150)) = 2^(151-e).
         *
         * The bit at the top of the first (zero) word of the above
         * array has place value 2^31. Hence, the bit we want to put
         * at the top of the first word we extract from that array
         * is the one at bit index n, where 31-n = 151-e and hence
         * n=e-120.
         */
        int topbitindex = ((k >> 23) & 0xFF) - 120;
        int wordindex = topbitindex >> 5;
        int shiftup = topbitindex & 31;
        int shiftdown = 32 - shiftup;
        unsigned word1, word2, word3;
        if (shiftup) {
            word1 = (twooverpi[wordindex] << shiftup) | (twooverpi[wordindex+1] >> shiftdown);
            word2 = (twooverpi[wordindex+1] << shiftup) | (twooverpi[wordindex+2] >> shiftdown);
            word3 = (twooverpi[wordindex+2] << shiftup) | (twooverpi[wordindex+3] >> shiftdown);
        } else {
            word1 = twooverpi[wordindex];
            word2 = twooverpi[wordindex+1];
            word3 = twooverpi[wordindex+2];
        }

        /*
         * Do the multiplications, and add them together.
         */
        unsigned long long mult1 = (unsigned long long)word1 * mantissa;
        unsigned long long mult2 = (unsigned long long)word2 * mantissa;
        unsigned long long mult3 = (unsigned long long)word3 * mantissa;

        unsigned /* bottom3 = (unsigned)mult3, */ top3 = (unsigned)(mult3 >> 32);
        unsigned bottom2 = (unsigned)mult2, top2 = (unsigned)(mult2 >> 32);
        unsigned bottom1 = (unsigned)mult1, top1 = (unsigned)(mult1 >> 32);

        unsigned out3, out2, out1, carry;

        out3 = top3 + bottom2; carry = (out3 < top3);
        out2 = top2 + bottom1 + carry; carry = carry ? (out2 <= top2) : (out2 < top2);
        out1 = top1 + carry;

        /*
         * The two words we multiplied to get mult1 had their top
         * bits at (respectively) place values 2^(151-e) and
         * 2^(e-127). The value of those two bits multiplied
         * together will have ended up in bit 62 (the
         * topmost-but-one bit) of mult1, i.e. bit 30 of out1.
         * Hence, that bit has place value 2^(151-e+e-127) = 2^24.
         * So the integer value that we want to output as q,
         * consisting of the bits with place values 2^1 and 2^0,
         * must be 23 and 24 bits below that, i.e. in bits 7 and 6
         * of out1.
         *
         * Or, at least, it will be once we add 1/2, to round to the
         * _nearest_ multiple of pi/2 rather than the next one down.
         */
        *q = (out1 + (1<<5)) >> 6;

        /*
         * Now we construct the output fraction, which is most
         * simply done in the VFP. We just extract three consecutive
         * bit strings from our chunk of binary data, convert them
         * to integers, equip each with an appropriate FP exponent,
         * add them together, and (don't forget) multiply back up by
         * pi/2. That way we don't have to work out ourselves where
         * the highest fraction bit ended up.
         *
         * Since our displacement from the nearest multiple of pi/2
         * can be positive or negative, the topmost of these three
         * values must be arranged with its 2^-1 bit at the very top
         * of the word, and then treated as a _signed_ integer.
         */
        {
            int i1 = (out1 << 26) | ((out2 >> 19) << 13);
            unsigned i2 = out2 << 13;
            unsigned i3 = out3;
            float f1 = i1, f2 = i2 * (1.0f/524288.0f), f3 = i3 * (1.0f/524288.0f/524288.0f);

            /*
             * Now f1+f2+f3 is a representation, potentially to
             * twice double precision, of 2^32 times ((2/pi)*x minus
             * some integer). So our remaining job is to multiply
             * back down by (pi/2)*2^-32, and convert back to one
             * single-precision output number.
             */

            /* Normalise to a prec-and-a-half representation... */
            float ftop = CLEARBOTTOMHALF(f1+f2+f3), fbot = f3-((ftop-f1)-f2);

            /* ... and multiply by a prec-and-a-half value of (pi/2)*2^-32. */
            float ret = (ftop * 0x1.92p-32F) + (ftop * 0x1.fb5444p-44F + fbot * 0x1.921fb6p-32F);

            /* Just before we return, take the input sign into account. */
            if (k & 0x80000000) {
                *q = 0x10000000 - *q;
                ret = -ret;
            }
            return ret;
        }
    }
}

#ifdef __cplusplus
} /* end of extern "C" */
#endif /* __cplusplus */

/* end of rredf.c */
