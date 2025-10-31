/*
 * Single-precision vector cexpipi function.
 *
 * Copyright (c) 2025, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "sv_math.h"
#include "sv_sincospif_common.h"
#include "test_defs.h"

/* Single-precision vector function allowing calculation of both sinpi
   and cospi in one function call, using shared argument reduction and
   separate low-order polynomials.
   Worst-case error for sin is 3.04 ULP:
   _ZGVsMxvl4l4_sincospif_sin(0x1.b51b8p-2) got 0x1.f28b5ep-1
					   want 0x1.f28b58p-1.
   Worst-case error for cos is 3.18 ULP:
   _ZGVsMxvl4l4_sincospif_cos(0x1.d341a8p-5) got 0x1.f7cd56p-1
					    want 0x1.f7cd5p-1.  */
svfloat32x2_t
_ZGVsMxv_cexpipif (svfloat32_t x, svbool_t pg)
{
  return sv_sincospif_inline (pg, x);
}

#if WANT_C23_TESTS
TEST_ULP (_ZGVsMxv_cexpipif_sin, 2.54)
TEST_ULP (_ZGVsMxv_cexpipif_cos, 2.68)
#  define SV_CEXPIPIF_INTERVAL(lo, hi, n)                                     \
    TEST_INTERVAL (_ZGVsMxv_cexpipif_sin, lo, hi, n)                          \
    TEST_INTERVAL (_ZGVsMxv_cexpipif_cos, lo, hi, n)
SV_CEXPIPIF_INTERVAL (0, 0x1p20, 500000)
SV_CEXPIPIF_INTERVAL (-0, -0x1p20, 500000)
SV_CEXPIPIF_INTERVAL (0x1p20, inf, 10000)
SV_CEXPIPIF_INTERVAL (-0x1p20, -inf, 10000)
#endif
CLOSE_SVE_ATTR
