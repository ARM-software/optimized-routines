/*
 * Double-precision vector atan2(x) function.
 *
 * Copyright (c) 2021-2022, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "v_math.h"
#if V_SUPPORTED

#include "atan_common.h"

#define PiOver2 v_f64 (0x1.921fb54442d18p+0)
#define SignMask v_u64 (0x8000000000000000)

/* Special cases i.e. 0, infinity, NaN (fall back to scalar calls).  */
VPCS_ATTR
__attribute__ ((noinline)) static v_f64_t
specialcase (v_f64_t y, v_f64_t x, v_f64_t ret, v_u64_t cmp)
{
  return v_call2_f64 (atan2, y, x, ret, cmp);
}

/* Returns 1 if input is the bit representation of 0, infinity or nan.  */
static inline v_u64_t
zeroinfnan (v_u64_t i)
{
  return v_cond_u64 (2 * i - 1 >= v_u64 (2 * asuint64 (INFINITY) - 1));
}

/* Fast implementation of vector atan2.
   Maximum observed error is 2.8 ulps:
   v_atan2(0x1.9651a429a859ap+5, 0x1.953075f4ee26p+5)
	got 0x1.92d628ab678ccp-1
       want 0x1.92d628ab678cfp-1.  */
VPCS_ATTR
v_f64_t V_NAME (atan2) (v_f64_t y, v_f64_t x)
{
  v_u64_t ix = v_as_u64_f64 (x);
  v_u64_t iy = v_as_u64_f64 (y);

  v_u64_t special_cases = zeroinfnan (ix) | zeroinfnan (iy);

  v_u64_t sign_x = ix & SignMask;
  v_u64_t sign_y = iy & SignMask;
  v_u64_t sign_xy = sign_x ^ sign_y;

  v_f64_t ax = v_abs_f64 (x);
  v_f64_t ay = v_abs_f64 (y);

  v_u64_t pred_xlt0 = x < 0.0;
  v_u64_t pred_aygtax = ay > ax;

  /* Set up z for call to atan.  */
  v_f64_t n = v_sel_f64 (pred_aygtax, -ax, ay);
  v_f64_t d = v_sel_f64 (pred_aygtax, ay, ax);
  v_f64_t z = v_div_f64 (n, d);

  /* Work out the correct shift.  */
  v_f64_t shift = v_sel_f64 (pred_xlt0, v_f64 (-2.0), v_f64 (0.0));
  shift = v_sel_f64 (pred_aygtax, shift + 1.0, shift);
  shift *= PiOver2;

  v_f64_t ret = eval_poly (z, z, shift);

  /* Account for the sign of x and y.  */
  ret = v_as_f64_u64 (v_as_u64_f64 (ret) ^ sign_xy);

  if (unlikely (v_any_u64 (special_cases)))
    {
      return specialcase (y, x, ret, special_cases);
    }

  return ret;
}
VPCS_ALIAS

#endif
