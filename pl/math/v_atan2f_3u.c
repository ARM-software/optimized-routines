/*
 * Single-precision vector atan2(x) function.
 *
 * Copyright (c) 2021-2023, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "v_math.h"
#include "pl_sig.h"
#include "pl_test.h"

#include "atanf_common.h"

/* Useful constants.  */
#define PiOver2 v_f32 (0x1.921fb6p+0f)
#define SignMask v_u32 (0x80000000)

/* Special cases i.e. 0, infinity and nan (fall back to scalar calls).  */
VPCS_ATTR
NOINLINE static float32x4_t
specialcase (float32x4_t y, float32x4_t x, float32x4_t ret, uint32x4_t cmp)
{
  return v_call2_f32 (atan2f, y, x, ret, cmp);
}

/* Returns 1 if input is the bit representation of 0, infinity or nan.  */
static inline uint32x4_t
zeroinfnan (uint32x4_t i)
{
  return v_cond_u32 (2 * i - 1 >= v_u32 (2 * 0x7f800000lu - 1));
}

/* Fast implementation of vector atan2f. Maximum observed error is
   2.95 ULP in [0x1.9300d6p+6 0x1.93c0c6p+6] x [0x1.8c2dbp+6 0x1.8cea6p+6]:
   v_atan2(0x1.93836cp+6, 0x1.8cae1p+6) got 0x1.967f06p-1
				       want 0x1.967f00p-1.  */
VPCS_ATTR
float32x4_t V_NAME (atan2f) (float32x4_t y, float32x4_t x)
{
  uint32x4_t ix = v_as_u32_f32 (x);
  uint32x4_t iy = v_as_u32_f32 (y);

  uint32x4_t special_cases = zeroinfnan (ix) | zeroinfnan (iy);

  uint32x4_t sign_x = ix & SignMask;
  uint32x4_t sign_y = iy & SignMask;
  uint32x4_t sign_xy = sign_x ^ sign_y;

  float32x4_t ax = v_abs_f32 (x);
  float32x4_t ay = v_abs_f32 (y);

  uint32x4_t pred_xlt0 = x < 0.0f;
  uint32x4_t pred_aygtax = ay > ax;

  /* Set up z for call to atanf.  */
  float32x4_t n = v_sel_f32 (pred_aygtax, -ax, ay);
  float32x4_t d = v_sel_f32 (pred_aygtax, ay, ax);
  float32x4_t z = v_div_f32 (n, d);

  /* Work out the correct shift.  */
  float32x4_t shift = v_sel_f32 (pred_xlt0, v_f32 (-2.0f), v_f32 (0.0f));
  shift = v_sel_f32 (pred_aygtax, shift + 1.0f, shift);
  shift *= PiOver2;

  float32x4_t ret = eval_poly (z, z, shift);

  /* Account for the sign of y.  */
  ret = v_as_f32_u32 (v_as_u32_f32 (ret) ^ sign_xy);

  if (unlikely (v_any_u32 (special_cases)))
    {
      return specialcase (y, x, ret, special_cases);
    }

  return ret;
}
PL_ALIAS (V_NAME (atan2f), _ZGVnN4vv_atan2f)

/* Arity of 2 means no mathbench entry emitted. See test/mathbench_funcs.h.  */
PL_SIG (V, F, 2, atan2)
PL_TEST_ULP (V_NAME (atan2f), 2.46)
PL_TEST_INTERVAL (V_NAME (atan2f), -10.0, 10.0, 50000)
PL_TEST_INTERVAL (V_NAME (atan2f), -1.0, 1.0, 40000)
PL_TEST_INTERVAL (V_NAME (atan2f), 0.0, 1.0, 40000)
PL_TEST_INTERVAL (V_NAME (atan2f), 1.0, 100.0, 40000)
PL_TEST_INTERVAL (V_NAME (atan2f), 1e6, 1e32, 40000)
