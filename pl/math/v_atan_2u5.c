/*
 * Double-precision vector atan(x) function.
 *
 * Copyright (c) 2021-2022, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "v_math.h"
#if V_SUPPORTED

#include "atan_common.h"

#define PiOver2 v_f64 (0x1.921fb54442d18p+0)
#define AbsMask v_u64 (0x7fffffffffffffff)

/* Fast implementation of vector atan.
   Based on atan(x) ~ shift + z + z^3 * P(z^2) with reduction to [0,1] using
   z=1/x and shift = pi/2. Maximum observed error is 2.27 ulps:
   __v_atan(0x1.0005af27c23e9p+0) got 0x1.9225645bdd7c1p-1
				 want 0x1.9225645bdd7c3p-1.  */
VPCS_ATTR
v_f64_t V_NAME (atan) (v_f64_t x)
{
  /* No need to trigger special case. Small cases, infs and nans
     are supported by our approximation technique.  */
  v_u64_t ix = v_as_u64_f64 (x);
  v_u64_t sign = ix & ~AbsMask;

  /* Argument reduction:
     y := arctan(x) for x < 1
     y := pi/2 + arctan(-1/x) for x > 1
     Hence, use z=-1/a if x>=1, otherwise z=a.  */
  v_u64_t red = v_cagt_f64 (x, v_f64 (1.0));
  /* Avoid dependency in abs(x) in division (and comparison).  */
  v_f64_t z = v_sel_f64 (red, v_div_f64 (v_f64 (-1.0), x), x);
  v_f64_t shift = v_sel_f64 (red, PiOver2, v_f64 (0.0));
  /* Use absolute value only when needed (odd powers of z).  */
  v_f64_t az = v_abs_f64 (z);
  az = v_sel_f64 (red, -az, az);

  /* Calculate the polynomial approximation.  */
  v_f64_t y = eval_poly (z, az, shift);

  /* y = atan(x) if x>0, -atan(-x) otherwise.  */
  y = v_as_f64_u64 (v_as_u64_f64 (y) ^ sign);

  return y;
}
VPCS_ALIAS
#endif
