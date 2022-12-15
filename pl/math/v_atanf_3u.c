/*
 * Single-precision vector atan(x) function.
 *
 * Copyright (c) 2021-2022, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "v_math.h"
#include "pl_sig.h"
#include "pl_test.h"

#if V_SUPPORTED

#include "atanf_common.h"

#define PiOver2 v_f32 (0x1.921fb6p+0f)
#define AbsMask v_u32 (0x7fffffff)

/* Fast implementation of vector atanf based on
   atan(x) ~ shift + z + z^3 * P(z^2) with reduction to [0,1]
   using z=-1/x and shift = pi/2. Maximum observed error is 2.9ulps:
   v_atanf(0x1.0468f6p+0) got 0x1.967f06p-1 want 0x1.967fp-1.  */
VPCS_ATTR
v_f32_t V_NAME (atanf) (v_f32_t x)
{
  /* No need to trigger special case. Small cases, infs and nans
     are supported by our approximation technique.  */
  v_u32_t ix = v_as_u32_f32 (x);
  v_u32_t sign = ix & ~AbsMask;

  /* Argument reduction:
     y := arctan(x) for x < 1
     y := pi/2 + arctan(-1/x) for x > 1
     Hence, use z=-1/a if x>=1, otherwise z=a.  */
  v_u32_t red = v_cagt_f32 (x, v_f32 (1.0));
  /* Avoid dependency in abs(x) in division (and comparison).  */
  v_f32_t z = v_sel_f32 (red, v_div_f32 (v_f32 (-1.0f), x), x);
  v_f32_t shift = v_sel_f32 (red, PiOver2, v_f32 (0.0f));
  /* Use absolute value only when needed (odd powers of z).  */
  v_f32_t az = v_abs_f32 (z);
  az = v_sel_f32 (red, -az, az);

  /* Calculate the polynomial approximation.  */
  v_f32_t y = eval_poly (z, az, shift);

  /* y = atan(x) if x>0, -atan(-x) otherwise.  */
  y = v_as_f32_u32 (v_as_u32_f32 (y) ^ sign);

  return y;
}
VPCS_ALIAS

PL_SIG (V, F, 1, atan, -10.0, 10.0)
PL_TEST_ULP (V_NAME (atanf), 2.5)
PL_TEST_INTERVAL (V_NAME (atanf), -10.0, 10.0, 50000)
PL_TEST_INTERVAL (V_NAME (atanf), -1.0, 1.0, 40000)
PL_TEST_INTERVAL (V_NAME (atanf), 0.0, 1.0, 40000)
PL_TEST_INTERVAL (V_NAME (atanf), 1.0, 100.0, 40000)
PL_TEST_INTERVAL (V_NAME (atanf), 1e6, 1e32, 40000)
#endif
