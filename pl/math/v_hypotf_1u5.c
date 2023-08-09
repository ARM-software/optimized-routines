/*
 * Single-precision vector hypot(x) function.
 *
 * Copyright (c) 2023, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "v_math.h"
#include "pl_sig.h"
#include "pl_test.h"

static const struct data
{
  uint32x4_t tiny_bound, thres;
} data = {
  .tiny_bound = V4 (0x20000000), /* asuint (0x1p-63).  */
#if WANT_SIMD_EXCEPT
  .thres = V4 (0x3f000000), /* asuint (0x1p63) - tiny_bound.  */
#else
  .thres = V4 (0x5f800000), /* asuint (inf) - tiny_bound.  */
#endif
};

static float32x4_t VPCS_ATTR NOINLINE
special_case (float32x4_t x, float32x4_t y, float32x4_t ret,
	      uint32x4_t special)
{
  return v_call2_f32 (hypotf, x, y, ret, special);
}

/* Vector implementation of single-precision hypot.
   Maximum error observed is 1.21 ULP:
   _ZGVnN4vv_hypotf (0x1.6a419cp-13, 0x1.82a852p-22) got 0x1.6a41d2p-13
						    want 0x1.6a41dp-13.  */
float32x4_t VPCS_ATTR V_NAME_F2 (hypot) (float32x4_t x, float32x4_t y)
{
  const struct data *d = ptr_barrier (&data);

  float32x4_t ax = x;
  float32x4_t ay = y;

#if WANT_SIMD_EXCEPT
  ax = vabsq_f32 (ax);
  ay = vabsq_f32 (ay);

  uint32x4_t ix = vreinterpretq_u32_f32 (ax);
  uint32x4_t iy = vreinterpretq_u32_f32 (ay);

  /* Extreme values, NaNs, and infinities should be handled by the scalar
     fallback for correct flag handling.  */
  uint32x4_t special
      = vorrq_u32 (vcgeq_u32 (vsubq_u32 (ix, d->tiny_bound), d->thres),
		   vcgeq_u32 (vsubq_u32 (iy, d->tiny_bound), d->thres));
  ax = vreinterpretq_f32_u32 (vbicq_u32 (ix, special));
  ay = vreinterpretq_f32_u32 (vbicq_u32 (iy, special));
#endif

  float32x4_t sqsum = vfmaq_f32 (vmulq_f32 (ax, ax), ay, ay);

#if !WANT_SIMD_EXCEPT
  uint32x4_t special = vcgeq_u32 (
      vsubq_u32 (vreinterpretq_u32_f32 (sqsum), d->tiny_bound), d->thres);
#endif

  float32x4_t ret = vsqrtq_f32 (sqsum);

  if (unlikely (v_any_u32 (special)))
    return special_case (x, y, ret, special);

  return ret;
}

PL_SIG (V, F, 2, hypot, -10.0, 10.0)
PL_TEST_ULP (V_NAME_F2 (hypot), 1.21)
PL_TEST_EXPECT_FENV (V_NAME_F2 (hypot), WANT_SIMD_EXCEPT)
PL_TEST_INTERVAL2 (V_NAME_F2 (hypot), 0, inf, 0, inf, 10000)
PL_TEST_INTERVAL2 (V_NAME_F2 (hypot), 0, inf, -0, -inf, 10000)
PL_TEST_INTERVAL2 (V_NAME_F2 (hypot), -0, -inf, 0, inf, 10000)
PL_TEST_INTERVAL2 (V_NAME_F2 (hypot), -0, -inf, -0, -inf, 10000)
