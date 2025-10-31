/*
 * Double-precision vector cexpipi function.
 *
 * Copyright (c) 2025, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "sv_math.h"
#include "sv_sincospi_common.h"
#include "test_defs.h"

/* Double-precision vector function allowing calculation of both sinpi and
   cospi in one function call, using shared argument reduction and separate
   polynomials.
   Worst-case error for sin is 3.09 ULP:
   _ZGVsMxv_cexpipi_sin(0x1.7a41deb4b21e1p+14) got 0x1.fd54d0b327cf1p-1
					      want 0x1.fd54d0b327cf4p-1.
   Worst-case error for sin is 3.16 ULP:
   _ZGVsMxv_cexpipi_cos(-0x1.11e3c7e284adep-5) got 0x1.fd2da484ff3ffp-1
					      want 0x1.fd2da484ff402p-1.  */
svfloat64x2_t
_ZGVsMxv_cexpipi (svfloat64_t x, svbool_t pg)
{
  return sv_sincospi_inline (pg, x);
}

#if WANT_C23_TESTS
TEST_ULP (_ZGVsMxv_cexpipi_sin, 2.59)
TEST_ULP (_ZGVsMxv_cexpipi_cos, 2.66)
#  define SV_CEXPIPI_INTERVAL(lo, hi, n)                                      \
    TEST_INTERVAL (_ZGVsMxv_cexpipi_sin, lo, hi, n)                           \
    TEST_INTERVAL (_ZGVsMxv_cexpipi_cos, lo, hi, n)
SV_CEXPIPI_INTERVAL (0, 0x1p23, 500000)
SV_CEXPIPI_INTERVAL (-0, -0x1p23, 500000)
SV_CEXPIPI_INTERVAL (0x1p23, inf, 10000)
SV_CEXPIPI_INTERVAL (-0x1p23, -inf, 10000)
#endif
CLOSE_SVE_ATTR
