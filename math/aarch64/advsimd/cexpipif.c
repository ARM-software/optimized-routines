/*
 * Single-precision vector cexpipi function.
 *
 * Copyright (c) 2025, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "v_math.h"
#include "v_sincospif_common.h"
#include "test_defs.h"

/* Single-precision vector function allowing calculation of both sinpi and
   cospi in one function call, using shared argument reduction and separate
   low-order polynomials.
   Worst-case error for sin is 3.04 ULP:
   _ZGVnN4v_cexpipif_sin(0x1.1d341ap-1) got 0x1.f7cd56p-1
				       want 0x1.f7cd5p-1.
   Worst-case error for cos is 3.18 ULP:
   _ZGVnN4v_cexpipif_cos(0x1.d341a8p-5) got 0x1.f7cd56p-1
				       want 0x1.f7cd5p-1.  */
VPCS_ATTR float32x4x2_t
_ZGVnN4v_cexpipif (float32x4_t x)
{
  return v_sincospif_inline (x);
}

#if WANT_C23_TESTS
TEST_ULP (_ZGVnN4v_cexpipif_sin, 2.54)
TEST_ULP (_ZGVnN4v_cexpipif_cos, 2.68)
#  define V_CEXPIPIF_INTERVAL(lo, hi, n)                                      \
    TEST_INTERVAL (_ZGVnN4v_cexpipif_sin, lo, hi, n)                          \
    TEST_INTERVAL (_ZGVnN4v_cexpipif_cos, lo, hi, n)
V_CEXPIPIF_INTERVAL (0, 0x1p20, 500000)
V_CEXPIPIF_INTERVAL (-0, -0x1p20, 500000)
V_CEXPIPIF_INTERVAL (0x1p20, inf, 10000)
V_CEXPIPIF_INTERVAL (-0x1p20, -inf, 10000)
#endif
