/*
 * Single-precision vector powrf function.
 *
 * Copyright (c) 2025, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "v_math.h"
#include "test_defs.h"
#include "v_powf_inline.h"

/* Implementation of AdvSIMD powrf.
     powr(x,y) := exp(y * log (x))
   This means powr(x,y) core computation matches that of pow(x,y)
   but powr returns NaN for negative x even if y is an integer.
   The theoretical maximum error is under 2.60 ULPs.
   Maximum measured error is 2.57 ULPs:
   V_NAME_F2 (pow) (0x1.031706p+0, 0x1.ce2ec2p+12)
     got 0x1.fff868p+127
    want 0x1.fff862p+127.  */
float32x4_t VPCS_ATTR NOINLINE V_NAME_F2 (powr) (float32x4_t x, float32x4_t y)
{
  /* Current powf core treats x negative and y integer differently,
     for the sake of clarity we fix that in powr using extra operations.
     Following patches will rely on a simplified core instead.  */
  uint32x4_t pos = vcgezq_f32 (x);
  return vbslq_f32 (pos, v_powf_inline (x, y), v_f32 (__builtin_nanf ("")));
}

HALF_WIDTH_ALIAS_F2 (powr)

#if WANT_C23_TESTS
TEST_ULP (V_NAME_F2 (powr), 2.1)
#  define V_POWRF_INTERVAL2(xlo, xhi, ylo, yhi, n)                            \
    TEST_INTERVAL2 (V_NAME_F2 (powr), xlo, xhi, ylo, yhi, n)                  \
    TEST_INTERVAL2 (V_NAME_F2 (powr), xlo, xhi, -ylo, -yhi, n)
/* Wide intervals spanning the whole domain.  */
V_POWRF_INTERVAL2 (0, 0x1p-126, 0, inf, 40000)
V_POWRF_INTERVAL2 (0x1p-126, 1, 0, inf, 50000)
V_POWRF_INTERVAL2 (1, inf, 0, inf, 50000)
/* x~1 or y~1.  */
V_POWRF_INTERVAL2 (0x1p-1, 0x1p1, 0x1p-7, 0x1p7, 50000)
V_POWRF_INTERVAL2 (0x1p-70, 0x1p70, 0x1p-1, 0x1p1, 50000)
V_POWRF_INTERVAL2 (0x1.ep-1, 0x1.1p0, 0x1p8, 0x1p14, 50000)
/* |x| is 0, inf or nan.  */
V_POWRF_INTERVAL2 (0.0, 0.0, 0, inf, 1000)
V_POWRF_INTERVAL2 (inf, inf, 0, inf, 1000)
V_POWRF_INTERVAL2 (nan, nan, 0, inf, 1000)
/* |y| is 0, inf or nan.  */
V_POWRF_INTERVAL2 (0, inf, 0.0, 0.0, 1000)
V_POWRF_INTERVAL2 (0, inf, inf, inf, 1000)
V_POWRF_INTERVAL2 (0, inf, nan, nan, 1000)
/* x is negative.  */
TEST_INTERVAL2 (V_NAME_F2 (powr), -0.0, -inf, 0, 0xffff0000, 1000)
#endif
