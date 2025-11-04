/*
 * Single-precision vector powf function.
 *
 * Copyright (c) 2019-2025, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "v_math.h"
#include "test_defs.h"
#include "test_sig.h"
#include "v_powf_inline.h"

/* Implementation of AdvSIMD powf.
   The theoretical maximum error is under 2.60 ULPs.
   Maximum measured error is 2.57 ULPs:
   V_NAME_F2 (pow) (0x1.031706p+0, 0x1.ce2ec2p+12)
     got 0x1.fff868p+127
    want 0x1.fff862p+127.  */
float32x4_t VPCS_ATTR NOINLINE V_NAME_F2 (pow) (float32x4_t x, float32x4_t y)
{
  return v_powf_inline (x, y);
}

HALF_WIDTH_ALIAS_F2 (pow)

TEST_SIG (V, F, 2, pow)
TEST_ULP (V_NAME_F2 (pow), 2.1)
#define V_POWF_INTERVAL2(xlo, xhi, ylo, yhi, n)                               \
  TEST_INTERVAL2 (V_NAME_F2 (pow), xlo, xhi, ylo, yhi, n)                     \
  TEST_INTERVAL2 (V_NAME_F2 (pow), xlo, xhi, -ylo, -yhi, n)
/* Wide intervals spanning the whole domain.  */
V_POWF_INTERVAL2 (0, 0x1p-126, 0, inf, 40000)
V_POWF_INTERVAL2 (0x1p-126, 1, 0, inf, 50000)
V_POWF_INTERVAL2 (1, inf, 0, inf, 50000)
/* x~1 or y~1.  */
V_POWF_INTERVAL2 (0x1p-1, 0x1p1, 0x1p-7, 0x1p7, 50000)
V_POWF_INTERVAL2 (0x1p-70, 0x1p70, 0x1p-1, 0x1p1, 50000)
V_POWF_INTERVAL2 (0x1.ep-1, 0x1.1p0, 0x1p8, 0x1p14, 50000)
/* |x| is 0, inf or nan.  */
V_POWF_INTERVAL2 (0.0, 0.0, 0, inf, 1000)
V_POWF_INTERVAL2 (inf, inf, 0, inf, 1000)
V_POWF_INTERVAL2 (nan, nan, 0, inf, 1000)
/* |y| is 0, inf or nan.  */
V_POWF_INTERVAL2 (0, inf, 0.0, 0.0, 1000)
V_POWF_INTERVAL2 (0, inf, inf, inf, 1000)
V_POWF_INTERVAL2 (0, inf, nan, nan, 1000)
/* x is negative.  */
TEST_INTERVAL2 (V_NAME_F2 (powr), -0.0, -inf, 0, 0xffff0000, 1000)

/* pow-specific cases, not shared with powr.  */

/* x is negative, y is odd or even integer, or y is real not integer.  */
V_POWF_INTERVAL2 (-0.0, -10.0, 1.0, 1.0, 1000)
V_POWF_INTERVAL2 (-0.0, -10.0, 3.0, 3.0, 1000)
V_POWF_INTERVAL2 (-0.0, -10.0, 4.0, 4.0, 1000)
V_POWF_INTERVAL2 (-0.0, -10.0, 0.0, 10.0, 1000)
V_POWF_INTERVAL2 (0.0, 10.0, 0.0, 10.0, 1000)
/* |x| is inf, y is odd or even integer, or y is real not integer.  */
V_POWF_INTERVAL2 (inf, inf, 0.5, 0.5, 1)
V_POWF_INTERVAL2 (inf, inf, 1.0, 1.0, 1)
V_POWF_INTERVAL2 (inf, inf, 2.0, 2.0, 1)
V_POWF_INTERVAL2 (inf, inf, 3.0, 3.0, 1)
