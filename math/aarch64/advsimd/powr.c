/*
 * Double-precision vector powr function.
 *
 * Copyright (c) 2025, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "v_math.h"
#include "test_sig.h"
#include "test_defs.h"
#define WANT_V_POW_SIGN_BIAS 0
#include "v_pow_inline.h"

float64x2_t VPCS_ATTR V_NAME_D2 (powr) (float64x2_t x, float64x2_t y)
{
  return v_pow_inline (x, y);
}

#if WANT_C23_TESTS
TEST_ULP (V_NAME_D2 (powr), 0.55)
#  define V_POW_INTERVAL2(xlo, xhi, ylo, yhi, n)                              \
    TEST_INTERVAL2 (V_NAME_D2 (powr), xlo, xhi, ylo, yhi, n)                  \
    TEST_INTERVAL2 (V_NAME_D2 (powr), xlo, xhi, -ylo, -yhi, n)                \
    TEST_INTERVAL2 (V_NAME_D2 (powr), -xlo, -xhi, ylo, yhi, n)                \
    TEST_INTERVAL2 (V_NAME_D2 (powr), -xlo, -xhi, -ylo, -yhi, n)
#  define EXPAND(str) str##000000000
#  define SHL52(str) EXPAND (str)
/* Wide intervals spanning the whole domain.  */
V_POW_INTERVAL2 (0, SHL52 (SmallPowX), 0, inf, 40000)
V_POW_INTERVAL2 (SHL52 (SmallPowX), SHL52 (BigPowX), 0, inf, 40000)
V_POW_INTERVAL2 (SHL52 (BigPowX), inf, 0, inf, 40000)
V_POW_INTERVAL2 (0, inf, 0, SHL52 (SmallPowY), 40000)
V_POW_INTERVAL2 (0, inf, SHL52 (SmallPowY), SHL52 (BigPowY), 40000)
V_POW_INTERVAL2 (0, inf, SHL52 (BigPowY), inf, 40000)
V_POW_INTERVAL2 (0, inf, 0, inf, 1000)
/* x~1 or y~1.  */
V_POW_INTERVAL2 (0x1p-1, 0x1p1, 0x1p-10, 0x1p10, 10000)
V_POW_INTERVAL2 (0x1p-500, 0x1p500, 0x1p-1, 0x1p1, 10000)
V_POW_INTERVAL2 (0x1.ep-1, 0x1.1p0, 0x1p8, 0x1p16, 10000)
/* around argmaxs of ULP error.  */
V_POW_INTERVAL2 (0x1p-300, 0x1p-200, 0x1p-20, 0x1p-10, 10000)
V_POW_INTERVAL2 (0x1p50, 0x1p100, 0x1p-20, 0x1p-10, 10000)
/* Common intervals with constrained signs.  */
TEST_INTERVAL2 (V_NAME_D2 (powr), -0.0, -10.0, 0.0, 10.0, 10000)
TEST_INTERVAL2 (V_NAME_D2 (powr), 0.0, 10.0, -0.0, -10.0, 10000)
/* 1.0^y.  */
TEST_INTERVAL2 (V_NAME_D2 (powr), 1.0, 1.0, 0.0, 0x1p-50, 1000)
TEST_INTERVAL2 (V_NAME_D2 (powr), 1.0, 1.0, 0x1p-50, 1.0, 1000)
TEST_INTERVAL2 (V_NAME_D2 (powr), 1.0, 1.0, 1.0, 0x1p100, 1000)
TEST_INTERVAL2 (V_NAME_D2 (powr), 1.0, 1.0, -1.0, -0x1p120, 1000)
#endif
