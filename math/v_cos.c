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
  float64x2_t poly[8];
  float64x2_t range_val, shift, inv_pi, half_pi, pi_1, pi_2, pi_3;
} data =
{
  /* Worst-case error is 1.514 ulp in [-pi/2, pi/2].  */
  .poly =
    {
      V2 (-0x1.5555555555555p-3),
      V2 (0x1.11111111110c1p-7),
      V2 (-0x1.a01a01a01443bp-13),
      V2 (0x1.71de3a52583dap-19),
      V2 (-0x1.ae6454b14b527p-26),
      V2 (0x1.6123c3ff0435cp-33),
      V2 (-0x1.ae4128fa984ddp-41),
      V2 (0x1.87f39c7fd3424p-49)
    },

  .inv_pi = V2 (0x1.45f306dc9c883p-2),
  .half_pi = V2 (0x1.921fb54442d18p+0),
  .pi_1 = V2 (0x1.921fb54442d18p+1),
  .pi_2 = V2 (0x1.1a62633145c06p-53),
  .pi_3 = V2 (0x1.c1cd129024e09p-106),
  .shift = V2 (0x1.8p52),
  .range_val = V2 (0x1p23)
};

#define C(i) data.poly[i]

static float64x2_t VPCS_ATTR NOINLINE
special_case (float64x2_t x, float64x2_t y, uint64x2_t odd, uint64x2_t cmp)
{
  y = vreinterpretq_f64_u64 (veorq_u64 (vreinterpretq_u64_f64 (y), odd));
  return v_call_f64 (cos, x, y, cmp);
}

float64x2_t VPCS_ATTR V_NAME_D1 (cos) (float64x2_t x)
{
  float64x2_t n, r, r2, r3, r4, t0, t1, t2, t3, y;
  uint64x2_t odd, cmp;

  cmp = vcageq_f64 (data.range_val, x);
  cmp = vceqzq_u64 (cmp);	/* cmp = ~cmp.  */
  r = x;

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
  r3 = vmulq_f64 (r2, r);
  r4 = vmulq_f64 (r2, r2);

  t0 = vfmaq_f64 (C (6), C (7), r2);
  t1 = vfmaq_f64 (C (4), C (5), r2);
  t2 = vfmaq_f64 (C (2), C (3), r2);
  t3 = vfmaq_f64 (C (0), C (1), r2);

  y = vfmaq_f64 (t1, t0, r4);
  y = vfmaq_f64 (t2, y, r4);
  y = vfmaq_f64 (t3, y, r4);
  y = vfmaq_f64 (r, y, r3);

  if (unlikely (v_any_u64 (cmp)))
    return special_case (x, y, odd, cmp);
  return vreinterpretq_f64_u64 (veorq_u64 (vreinterpretq_u64_f64 (y), odd));
}
