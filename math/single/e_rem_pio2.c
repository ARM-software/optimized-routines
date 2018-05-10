/*
 * e_rem_pio2.c
 *
 * Copyright (c) 1999-2015, Arm Limited.
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

#include <math.h>
#include "math_private.h"

int __ieee754_rem_pio2(double x, double *y) {
    int q;

    y[1] = 0.0;                        /* default */

    /*
     * Simple cases: all nicked from the fdlibm version for speed.
     */
    {
        static const double invpio2 =  0x1.45f306dc9c883p-1;
        static const double pio2s[] = {
            0x1.921fb544p+0, /* 1.57079632673412561417e+00 */
            0x1.0b4611a626331p-34, /* 6.07710050650619224932e-11 */
            0x1.0b4611a6p-34, /* 6.07710050630396597660e-11 */
            0x1.3198a2e037073p-69, /* 2.02226624879595063154e-21 */
            0x1.3198a2ep-69, /* 2.02226624871116645580e-21 */
            0x1.b839a252049c1p-104, /* 8.47842766036889956997e-32 */
        };

        double z,w,t,r,fn;
        int i,j,idx,n,ix,hx;

        hx = __HI(x);           /* high word of x */
        ix = hx&0x7fffffff;
        if(ix<=0x3fe921fb)   /* |x| ~<= pi/4 , no need for reduction */
            {y[0] = x; y[1] = 0; return 0;}
        if(ix<0x4002d97c) {  /* |x| < 3pi/4, special case with n=+-1 */
            if(hx>0) {
                z = x - pio2s[0];
                if(ix!=0x3ff921fb) {    /* 33+53 bit pi is good enough */
                    y[0] = z - pio2s[1];
#ifdef bottomhalf
                    y[1] = (z-y[0])-pio2s[1];
#endif
                } else {                /* near pi/2, use 33+33+53 bit pi */
                    z -= pio2s[2];
                    y[0] = z - pio2s[3];
#ifdef bottomhalf
                    y[1] = (z-y[0])-pio2s[3];
#endif
                }
                return 1;
            } else {    /* negative x */
                z = x + pio2s[0];
                if(ix!=0x3ff921fb) {    /* 33+53 bit pi is good enough */
                    y[0] = z + pio2s[1];
#ifdef bottomhalf
                    y[1] = (z-y[0])+pio2s[1];
#endif
                } else {                /* near pi/2, use 33+33+53 bit pi */
                    z += pio2s[2];
                    y[0] = z + pio2s[3];
#ifdef bottomhalf
                    y[1] = (z-y[0])+pio2s[3];
#endif
                }
                return -1;
            }
        }
        if(ix<=0x413921fb) { /* |x| ~<= 2^19*(pi/2), medium size */
            t  = fabs(x);
            n  = (int) (t*invpio2+0.5);
            fn = (double)n;
            r  = t-fn*pio2s[0];
            w  = fn*pio2s[1];    /* 1st round good to 85 bit */
            j  = ix>>20;
            idx = 1;
            while (1) {
                y[0] = r-w;
                if (idx == 3)
                    break;
                i = j-(((__HI(y[0]))>>20)&0x7ff);
                if(i <= 33*idx-17)
                    break;
                t  = r;
                w  = fn*pio2s[2*idx];
                r  = t-w;
                w  = fn*pio2s[2*idx+1]-((t-r)-w);
                idx++;
            }
#ifdef bottomhalf
            y[1] = (r-y[0])-w;
#endif
            if(hx<0) {
                y[0] = -y[0];
#ifdef bottomhalf
                y[1] = -y[1];
#endif
                return -n;
            } else {
                return n;
            }
        }
    }

    {
        static const unsigned twooverpi[] = {
            /* We start with two zero words, because they take up less
             * space than the array bounds checking and special-case
             * handling that would have to occur in their absence. */
            0, 0,
            /* 2/pi in hex is 0.a2f9836e... */
            0xa2f9836e, 0x4e441529, 0xfc2757d1, 0xf534ddc0, 0xdb629599,
            0x3c439041, 0xfe5163ab, 0xdebbc561, 0xb7246e3a, 0x424dd2e0,
            0x06492eea, 0x09d1921c, 0xfe1deb1c, 0xb129a73e, 0xe88235f5,
            0x2ebb4484, 0xe99c7026, 0xb45f7e41, 0x3991d639, 0x835339f4,
            0x9c845f8b, 0xbdf9283b, 0x1ff897ff, 0xde05980f, 0xef2f118b,
            0x5a0a6d1f, 0x6d367ecf, 0x27cb09b7, 0x4f463f66, 0x9e5fea2d,
            0x7527bac7, 0xebe5f17b, 0x3d0739f7, 0x8a5292ea, 0x6bfb5fb1,
            0x1f8d5d08,
        };

        /*
         * Multiprecision multiplication of this nature is more
         * readily done in integers than in VFP, since we can use
         * UMULL (on CPUs that support it) to multiply 32 by 32 bits
         * at a time whereas the VFP would only be able to do 12x12
         * without losing accuracy.
         *
         * So extract the mantissa of the input number as two 32-bit
         * integers.
         */
        unsigned mant1 = 0x00100000 | (__HI(x) & 0xFFFFF);
        unsigned mant2 = __LO(x);
        /*
         * Now work out which part of our stored value of 2/pi we're
         * supposed to be multiplying by.
         *
         * Let the IEEE exponent field of x be e. With its bias
         * removed, (e-1023) is the index of the set bit in bit 20
         * of 'mant1' (i.e. that set bit has real place value
         * 2^(e-1023)). So the lowest set bit in 'mant1', 52 bits
         * further down, must have place value 2^(e-1075).
         *
         * We begin taking an interest in the value of 2/pi at the
         * bit which multiplies by _that_ to give something with
         * place value at most 2. In other words, the highest bit of
         * 2/pi we're interested in is the one with place value
         * 2/(2^(e-1075)) = 2^(1076-e).
         *
         * The bit at the top of the first zero word of the above
         * array has place value 2^63. Hence, the bit we want to put
         * at the top of the first word we extract from that array
         * is the one at bit index n, where 63-n = 1076-e and hence
         * n=e-1013.
         */
        int topbitindex = ((__HI(x) >> 20) & 0x7FF) - 1013;
        int wordindex = topbitindex >> 5;
        int shiftup = topbitindex & 31;
        int shiftdown = 32 - shiftup;
        unsigned scaled[8];
        int i;

        scaled[7] = scaled[6] = 0;

        for (i = 6; i-- > 0 ;) {
            /*
             * Extract a word from our representation of 2/pi.
             */
            unsigned word;
            if (shiftup)
                word = (twooverpi[wordindex + i] << shiftup) | (twooverpi[wordindex + i + 1] >> shiftdown);
            else
                word = twooverpi[wordindex + i];
            /*
             * Multiply it by both words of our mantissa. (Should
             * generate UMULLs where available.)
             */
            unsigned long long mult1 = (unsigned long long)word * mant1;
            unsigned long long mult2 = (unsigned long long)word * mant2;
            /*
             * Split those up into 32-bit chunks.
             */
            unsigned bottom1 = (unsigned)mult1, top1 = (unsigned)(mult1 >> 32);
            unsigned bottom2 = (unsigned)mult2, top2 = (unsigned)(mult2 >> 32);
            /*
             * Add those two chunks together.
             */
            unsigned out1, out2, out3;

            out3 = bottom2;
            out2 = top2 + bottom1;
            out1 = top1 + (out2 < top2);
            /*
             * And finally add them to our 'scaled' array.
             */
            unsigned s3 = scaled[i+2], s2 = scaled[i+1], s1;
            unsigned carry;
            s3 = out3 + s3; carry = (s3 < out3);
            s2 = out2 + s2 + carry; carry = carry ? (s2 <= out2) : (s2 < out2);
            s1 = out1 + carry;

            scaled[i+2] = s3;
            scaled[i+1] = s2;
            scaled[i] = s1;
        }


        /*
         * The topmost set bit in mant1 is bit 20, and that has
         * place value 2^(e-1023). The topmost bit (bit 31) of the
         * most significant word we extracted from our twooverpi
         * array had place value 2^(1076-e). So the product of those
         * two bits must have place value 2^53; and that bit will
         * have ended up as bit 19 of scaled[0]. Hence, the integer
         * quadrant value we want to output, consisting of the bits
         * with place values 2^1 and 2^0, must be 52 and 53 bits
         * below that, i.e. precisely the topmost two bits of
         * scaled[2].
         *
         * Or, at least, it will be once we add 1/2, to round to the
         * _nearest_ multiple of pi/2 rather than the next one down.
         */
        q = (scaled[2] + (1<<29)) >> 30;

        /*
         * Now we construct the output fraction, which is most
         * simply done in the VFP. We just extract four consecutive
         * bit strings from our chunk of binary data, convert them
         * to doubles, equip each with an appropriate FP exponent,
         * add them together, and (don't forget) multiply back up by
         * pi/2. That way we don't have to work out ourselves where
         * the highest fraction bit ended up.
         *
         * Since our displacement from the nearest multiple of pi/2
         * can be positive or negative, the topmost of these four
         * values must be arranged with its 2^-1 bit at the very top
         * of the word, and then treated as a _signed_ integer.
         */
        int i1 = (scaled[2] << 2);
        unsigned i2 = scaled[3];
        unsigned i3 = scaled[4];
        unsigned i4 = scaled[5];
        double f1 = i1, f2 = i2 * (0x1.0p-30), f3 = i3 * (0x1.0p-62), f4 = i4 * (0x1.0p-94);

        /*
         * Now f1+f2+f3+f4 is a representation, potentially to
         * twice double precision, of 2^32 times ((2/pi)*x minus
         * some integer). So our remaining job is to multiply
         * back down by (pi/2)*2^-32, and convert back to one
         * single-precision output number.
         */
        double ftop = f1 + (f2 + (f3 + f4));
        ftop = __SET_LO(ftop, 0);
        double fbot = f4 - (((ftop-f1)-f2)-f3);

        /* ... and multiply by a prec-and-a-half value of (pi/2)*2^-32. */
        double ret = (ftop * 0x1.921fb54p-32) + (ftop * 0x1.10b4611a62633p-62 + fbot * 0x1.921fb54442d18p-32);

        /* Just before we return, take the input sign into account. */
        if (__HI(x) & 0x80000000) {
            q = -q;
            ret = -ret;
        }
        y[0] = ret;
        return q;
    }
}
