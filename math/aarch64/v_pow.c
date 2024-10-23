/*
 * Double-precision vector pow function.
 *
 * Copyright (c) 2020-2024, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "mathlib.h"
#include "v_math.h"
#include "test_defs.h"

float64x2_t VPCS_ATTR V_NAME_D2 (pow) (float64x2_t x, float64x2_t y)
{
  float64x2_t z;
  for (int lane = 0; lane < v_lanes64 (); lane++)
    {
      double sx = x[lane];
      double sy = y[lane];
      double sz = pow (sx, sy);
      z[lane] = sz;
    }
  return z;
}

TEST_ULP (V_NAME_D2 (pow), 0.05)
TEST_INTERVAL2 (V_NAME_D2 (pow), 0x1p-1, 0x1p1, 0x1p-10, 0x1p10, 50000)
TEST_INTERVAL2 (V_NAME_D2 (pow), 0x1p-1, 0x1p1, -0x1p-10, -0x1p10, 50000)
TEST_INTERVAL2 (V_NAME_D2 (pow), 0x1p-500, 0x1p500, 0x1p-1, 0x1p1, 50000)
TEST_INTERVAL2 (V_NAME_D2 (pow), 0x1p-500, 0x1p500, -0x1p-1, -0x1p1, 50000)
TEST_INTERVAL2 (V_NAME_D2 (pow), 0x1.ep-1, 0x1.1p0, 0x1p8, 0x1p16, 50000)
TEST_INTERVAL2 (V_NAME_D2 (pow), 0x1.ep-1, 0x1.1p0, -0x1p8, -0x1p16, 50000)
