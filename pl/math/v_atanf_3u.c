/*
 * Single-precision vector atan(x) function.
 *
 * Copyright (c) 2021-2023, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "v_math.h"
#include "pl_sig.h"
#include "pl_test.h"

#include "atanf_common.h"

#define PiOver2 v_f32 (0x1.921fb6p+0f)
#define AbsMask v_u32 (0x7fffffff)
#define TinyBound 0x308 /* top12(asuint(0x1p-30)).  */
#define BigBound 0x4e8	/* top12(asuint(0x1p30)).  */

#if WANT_SIMD_EXCEPT
static NOINLINE float32x4_t
specialcase (float32x4_t x, float32x4_t y, uint32x4_t special)
{
  return v_call_f32 (atanf, x, y, special);
}
#endif

/* Fast implementation of vector atanf based on
   atan(x) ~ shift + z + z^3 * P(z^2) with reduction to [0,1]
   using z=-1/x and shift = pi/2. Maximum observed error is 2.9ulps:
   v_atanf(0x1.0468f6p+0) got 0x1.967f06p-1 want 0x1.967fp-1.  */
VPCS_ATTR
float32x4_t V_NAME_F1 (atan) (float32x4_t x)
{
  /* Small cases, infs and nans are supported by our approximation technique,
     but do not set fenv flags correctly. Only trigger special case if we need
     fenv.  */
  uint32x4_t ix = v_as_u32_f32 (x);
  uint32x4_t sign = ix & ~AbsMask;

#if WANT_SIMD_EXCEPT
  uint32x4_t ia12 = (ix >> 20) & 0x7ff;
  uint32x4_t special = ia12 - TinyBound > BigBound - TinyBound;
  /* If any lane is special, fall back to the scalar routine for all lanes.  */
  if (unlikely (v_any_u32 (special)))
    return specialcase (x, x, v_u32 (-1));
#endif

  /* Argument reduction:
     y := arctan(x) for x < 1
     y := pi/2 + arctan(-1/x) for x > 1
     Hence, use z=-1/a if x>=1, otherwise z=a.  */
  uint32x4_t red = vcagtq_f32 (x, v_f32 (1.0));
  /* Avoid dependency in abs(x) in division (and comparison).  */
  float32x4_t z = vbslq_f32 (red, vdivq_f32 (v_f32 (-1.0f), x), x);
  float32x4_t shift = vbslq_f32 (red, PiOver2, v_f32 (0.0f));
  /* Use absolute value only when needed (odd powers of z).  */
  float32x4_t az = vabsq_f32 (z);
  az = vbslq_f32 (red, -az, az);

  /* Calculate the polynomial approximation.  */
  float32x4_t y = eval_poly (z, az, shift);

  /* y = atan(x) if x>0, -atan(-x) otherwise.  */
  y = v_as_f32_u32 (v_as_u32_f32 (y) ^ sign);

  return y;
}

PL_SIG (V, F, 1, atan, -10.0, 10.0)
PL_TEST_ULP (V_NAME_F1 (atan), 2.5)
PL_TEST_EXPECT_FENV (V_NAME_F1 (atan), WANT_SIMD_EXCEPT)
PL_TEST_INTERVAL (V_NAME_F1 (atan), 0, 0x1p-30, 5000)
PL_TEST_INTERVAL (V_NAME_F1 (atan), -0, -0x1p-30, 5000)
PL_TEST_INTERVAL (V_NAME_F1 (atan), 0x1p-30, 1, 40000)
PL_TEST_INTERVAL (V_NAME_F1 (atan), -0x1p-30, -1, 40000)
PL_TEST_INTERVAL (V_NAME_F1 (atan), 1, 0x1p30, 40000)
PL_TEST_INTERVAL (V_NAME_F1 (atan), -1, -0x1p30, 40000)
PL_TEST_INTERVAL (V_NAME_F1 (atan), 0x1p30, inf, 1000)
PL_TEST_INTERVAL (V_NAME_F1 (atan), -0x1p30, -inf, 1000)
