/*
 * Double-precision vector sincos function - return-by-value interface.
 *
 * Copyright (c) 2025, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "v_math.h"
#include "v_sincospi_common.h"
#include "test_defs.h"

/* Double-precision vector function allowing calculation of both sinpi and
  cospi in one function call, using shared argument reduction and separate
  polynomials.
  Maximum Error 3.09 ULP:
  _ZGVnN2v_cexpipi_sin(0x1.7a41deb4b21e1p+14) got 0x1.fd54d0b327cf1p-1
					     want 0x1.fd54d0b327cf4p-1
   Maximum Error 3.16 ULP:
  _ZGVnN2v_cexpipi_cos(-0x1.11e3c7e284adep-5) got 0x1.fd2da484ff3ffp-1
					     want 0x1.fd2da484ff402p-1.  */
VPCS_ATTR float64x2x2_t
_ZGVnN2v_cexpipi (float64x2_t x)
{
  return v_sincospi_inline (x);
}

#if WANT_C23_TESTS
TEST_ULP (_ZGVnN2v_cexpipi_sin, 2.59)
TEST_ULP (_ZGVnN2v_cexpipi_cos, 2.66)
#  define V_CEXPIPI_INTERVAL(lo, hi, n)                                       \
    TEST_INTERVAL (_ZGVnN2v_cexpipi_sin, lo, hi, n)                           \
    TEST_INTERVAL (_ZGVnN2v_cexpipi_cos, lo, hi, n)
V_CEXPIPI_INTERVAL (0, 0x1p23, 500000)
V_CEXPIPI_INTERVAL (-0, -0x1p23, 500000)
V_CEXPIPI_INTERVAL (0x1p23, inf, 10000)
V_CEXPIPI_INTERVAL (-0x1p23, -inf, 10000)
#endif
