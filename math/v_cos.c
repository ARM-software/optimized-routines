/*
 * Double-precision vector cos function.
 *
 * Copyright (c) 2019-2023, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "mathlib.h"
#include "v_math.h"

static const volatile struct __v_cos_data
{
  float64x2_t poly[7];
  float64x2_t inv_pi, half_pi, pi_1, pi_2, pi_3, shift;
} data = {
  /* worst-case error is 3.5 ulp.
     abs error: 0x1.be222a58p-53 in [-pi/2, pi/2].  */
  .poly = {V2 (-0x1.55555555554c3p-3), V2 (0x1.111111110b25ep-7),
	   V2 (-0x1.a01a019aeb4ffp-13), V2 (0x1.71de382e8d62bp-19),
	   V2 (-0x1.ae6361b7254e7p-26), V2 (0x1.60e88a10163f2p-33),
	   V2 (-0x1.9f4a9c8b21dc9p-41)},

  .inv_pi = V2 (0x1.45f306dc9c883p-2),
  .half_pi = V2 (0x1.921fb54442d18p+0),
  .pi_1 = V2 (0x1.921fb54442d18p+1),
  .pi_2 = V2 (0x1.1a62633145c06p-53),
  .pi_3 = V2 (0x1.c1cd129024e09p-106),
  .shift = V2 (0x1.8p52),
};

#define RangeVal v_u64 (0x4160000000000000) /* asuint64(0x1p23).  */
#define C(i) data.poly[i]

static float64x2_t VPCS_ATTR NOINLINE
special_case (float64x2_t x, float64x2_t y, uint64x2_t cmp)
{
  return v_call_f64 (cos, x, y, cmp);
}

float64x2_t VPCS_ATTR V_NAME (cos) (float64x2_t x)
{
  float64x2_t n, r, r2, y;
  uint64x2_t odd, cmp;

  r = vabsq_f64 (x);
  cmp = vcgeq_u64 (vreinterpretq_u64_f64 (r), RangeVal);

#if WANT_SIMD_EXCEPT
  if (unlikely (v_any_u64 (cmp)))
    /* If fenv exceptions are to be triggered correctly, set any special lanes
       to 1 (which is neutral w.r.t. fenv). These lanes will be fixed by
       special-case handler later.  */
    r = vbslq_f64 (cmp, v_f64 (1.0), r);
#endif

  /* n = rint((|x|+pi/2)/pi) - 0.5.  */
  n = vfmaq_f64 (data.shift, data.inv_pi, vaddq_f64 (r, data.half_pi));
  odd = vshlq_n_u64 (vreinterpretq_u64_f64 (n), 63);
  n = vsubq_f64 (n, data.shift);
  n = vsubq_f64 (n, v_f64 (0.5));

  /* r = |x| - n*pi  (range reduction into -pi/2 .. pi/2).  */
  r = vfmsq_f64 (r, data.pi_1, n);
  r = vfmsq_f64 (r, data.pi_2, n);
  r = vfmsq_f64 (r, data.pi_3, n);

  /* sin(r) poly approx.  */
  r2 = vmulq_f64 (r, r);
  y = vfmaq_f64 (C (5), C (6), r2);
  y = vfmaq_f64 (C (4), y, r2);
  y = vfmaq_f64 (C (3), y, r2);
  y = vfmaq_f64 (C (2), y, r2);
  y = vfmaq_f64 (C (1), y, r2);
  y = vfmaq_f64 (C (0), y, r2);
  y = vfmaq_f64 (r, vmulq_f64 (y, r2), r);

  /* sign.  */
  y = vreinterpretq_f64_u64 (veorq_u64 (vreinterpretq_u64_f64 (y), odd));

  if (unlikely (v_any_u64 (cmp)))
    return special_case (x, y, cmp);
  return y;
}
VPCS_ALIAS
