/*
 * Double-precision vector pow function.
 *
 * Copyright (c) 2020-2023, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "mathlib.h"
#include "v_math.h"

float64x2_t VPCS_ATTR V_NAME_D2 (pow) (float64x2_t x, float64x2_t y)
{
  float64x2_t z;
  for (int lane = 0; lane < v_lanes64 (); lane++)
    {
      double sx = vgetq_lane_f64 (x, lane);
      double sy = vgetq_lane_f64 (y, lane);
      double sz = pow (sx, sy);
      z = vsetq_lane_f64 (sz, z, lane);
    }
  return z;
}
