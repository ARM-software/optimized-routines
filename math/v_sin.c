/*
 * Double-precision vector sin function.
 *
 * Copyright (c) 2019-2023, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */
#include "mathlib.h"
#include "v_math.h"

static const volatile struct __v_sin_data
{
  float64x2_t poly[7];
  float64x2_t inv_pi, pi_1, pi_2, pi_3, shift;
} data = {
  .poly = {/* worst-case error is 3.5 ulp.
	      abs error: 0x1.be222a58p-53 in [-pi/2, pi/2].  */
	   V2 (-0x1.55555555554c3p-3), V2 (0x1.111111110b25ep-7),
	   V2 (-0x1.a01a019aeb4ffp-13), V2 (0x1.71de382e8d62bp-19),
	   V2 (-0x1.ae6361b7254e7p-26), V2 (0x1.60e88a10163f2p-33),
	   V2 (-0x1.9f4a9c8b21dc9p-41)},

  .inv_pi = V2 (0x1.45f306dc9c883p-2),
  .pi_1 = V2 (0x1.921fb54442d18p+1),
  .pi_2 = V2 (0x1.1a62633145c06p-53),
  .pi_3 = V2 (0x1.c1cd129024e09p-106),
  .shift = V2 (0x1.8p52),
};

#if WANT_SIMD_EXCEPT
#define TinyBound v_u64 (0x2020000000000000) /* asuint64 (0x1p-509).  */
#define Thresh v_u64 (0x2140000000000000)    /* RangeVal - TinyBound.  */
#else
#define RangeVal v_u64 (0x4160000000000000) /* asuint64(0x1p23).  */
#endif

#define C(i) data.poly[i]

static float64x2_t VPCS_ATTR NOINLINE
special_case (float64x2_t x, float64x2_t y, uint64x2_t cmp)
{
  return v_call_f64 (sin, x, y, cmp);
}

float64x2_t VPCS_ATTR V_NAME (sin) (float64x2_t x)
{
  float64x2_t n, r, r2, y;
  uint64x2_t sign, odd, cmp, ir;

  r = vabsq_f64 (x);
  ir = vreinterpretq_u64_f64 (r);
  sign = veorq_u64 (ir, vreinterpretq_u64_f64 (x));

#if WANT_SIMD_EXCEPT
  /* Detect |x| <= 0x1p-509 or |x| >= RangeVal. If fenv exceptions are to be
     triggered correctly, set any special lanes to 1 (which is neutral w.r.t.
     fenv). These lanes will be fixed by special-case handler later.  */
  cmp = vcgeq_u64 (vsubq_u64 (ir, TinyBound), Thresh);
  if (unlikely (v_any_u64 (cmp)))
    r = vbslq_f64 (cmp, v_f64 (1), r);
#else
  cmp = vcgeq_u64 (ir, RangeVal);
#endif

  /* n = rint(|x|/pi).  */
  n = vfmaq_f64 (data.shift, data.inv_pi, r);
  odd = vshlq_n_u64 (vreinterpretq_u64_f64 (n), 63);
  n = vsubq_f64 (n, data.shift);

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
  y = vreinterpretq_f64_u64 (
    veorq_u64 (veorq_u64 (vreinterpretq_u64_f64 (y), sign), odd));

  if (unlikely (v_any_u64 (cmp)))
    return special_case (x, y, cmp);
  return y;
}
VPCS_ALIAS
