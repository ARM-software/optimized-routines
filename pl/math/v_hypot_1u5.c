/*
 * Double-precision vector hypot(x) function.
 *
 * Copyright (c) 2023, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "v_math.h"
#include "pl_sig.h"
#include "pl_test.h"

static const struct data
{
  uint64x2_t tiny_bound, thres;
} data = {
#if WANT_SIMD_EXCEPT
  .tiny_bound = V2 (0x2000000000000000), /* asuint (0x1p-511).  */
  .thres = V2 (0x3fe0000000000000), /* asuint (0x1p511) - tiny_bound.  */
#else
  .tiny_bound = V2 (0x0020000000000000), /* asuint (DBL_MIN * 2).  */
  .thres = V2 (0x7fd0000000000000),	 /* asuint (inf) - tiny_bound.  */
#endif
};

static float64x2_t VPCS_ATTR NOINLINE
special_case (float64x2_t x, float64x2_t y, float64x2_t ret,
	      uint64x2_t special)
{
  return v_call2_f64 (hypot, x, y, ret, special);
}

/* Vector implementation of double-precision hypot.
   Maximum error observed is 1.21 ULP:
   _ZGVnN2vv_hypot (0x1.6a1b193ff85b5p-204, 0x1.bc50676c2a447p-222)
    got 0x1.6a1b19400964ep-204
   want 0x1.6a1b19400964dp-204.  */
float64x2_t VPCS_ATTR V_NAME_D2 (hypot) (float64x2_t x, float64x2_t y)
{
  const struct data *d = ptr_barrier (&data);

  float64x2_t ax = x;
  float64x2_t ay = y;

#if WANT_SIMD_EXCEPT
  ax = vabsq_f64 (x);
  ay = vabsq_f64 (y);

  uint64x2_t ix = vreinterpretq_u64_f64 (ax);
  uint64x2_t iy = vreinterpretq_u64_f64 (ay);

  /* Extreme values, NaNs, and infinities should be handled by the scalar
     fallback for correct flag handling.  */
  uint64x2_t special
      = vorrq_u64 (vcgeq_u64 (vsubq_u64 (ix, d->tiny_bound), d->thres),
		   vcgeq_u64 (vsubq_u64 (iy, d->tiny_bound), d->thres));
  ax = vreinterpretq_f64_u64 (vbicq_u64 (ix, special));
  ay = vreinterpretq_f64_u64 (vbicq_u64 (iy, special));
#endif

  float64x2_t sqsum = vfmaq_f64 (vmulq_f64 (ax, ax), ay, ay);

#if !WANT_SIMD_EXCEPT
  uint64x2_t special = vcgeq_u64 (
      vsubq_u64 (vreinterpretq_u64_f64 (sqsum), d->tiny_bound), d->thres);
#endif

  float64x2_t ret = vsqrtq_f64 (sqsum);

  if (unlikely (v_any_u64 (special)))
    return special_case (x, y, ret, special);

  return ret;
}

PL_SIG (V, D, 2, hypot, -10.0, 10.0)
PL_TEST_ULP (V_NAME_D2 (hypot), 1.21)
PL_TEST_EXPECT_FENV (V_NAME_D2 (hypot), WANT_SIMD_EXCEPT)
PL_TEST_INTERVAL2 (V_NAME_D2 (hypot), 0, inf, 0, inf, 10000)
PL_TEST_INTERVAL2 (V_NAME_D2 (hypot), 0, inf, -0, -inf, 10000)
PL_TEST_INTERVAL2 (V_NAME_D2 (hypot), -0, -inf, 0, inf, 10000)
PL_TEST_INTERVAL2 (V_NAME_D2 (hypot), -0, -inf, -0, -inf, 10000)
