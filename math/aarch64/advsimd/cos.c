/*
 * Double-precision vector cos function.
 *
 * Copyright (c) 2019-2026, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "test_defs.h"
#include "test_sig.h"
#include "v_math.h"
#include "v_trig_fallback.h"

static const struct data
{
  float64x2_t poly[7];
  float64x2_t range_val, inv_pi, pi_1, pi_2, pi_3;
} data = {
  /* Worst-case error is 3.3 ulp in [-pi/2, pi/2].  */
  .poly = { V2 (-0x1.555555555547bp-3), V2 (0x1.1111111108a4dp-7),
	    V2 (-0x1.a01a019936f27p-13), V2 (0x1.71de37a97d93ep-19),
	    V2 (-0x1.ae633919987c6p-26), V2 (0x1.60e277ae07cecp-33),
	    V2 (-0x1.9e9540300a1p-41) },
  .inv_pi = V2 (0x1.45f306dc9c883p-2),
  .pi_1 = V2 (0x1.921fb54442d18p+1),
  .pi_2 = V2 (0x1.1a62633145c06p-53),
  .pi_3 = V2 (0x1.c1cd129024e09p-106),
  .range_val = V2 (0x1p23)
};

#define C(i) d->poly[i]

static float64x2_t VPCS_ATTR NOINLINE
special_case (float64x2_t x, float64x2_t y, uint64x2_t odd, uint64x2_t cmp)
{
  y = vreinterpretq_f64_u64 (veorq_u64 (vreinterpretq_u64_f64 (y), odd));
  return v_call_vpcs_f64 (v_cos_fallback, x, y, cmp);
}

/* Vector AdvSIMD cos approximation.
   Maximum observed error in the non-special domain (|x| < 2^23)
   is 2.77 + 0.5 ULP
   _ZGVnN2v_cos (0x1.ad06044746e06p-2) got 0x1.d3b778d480fd6p-1
				      want 0x1.d3b778d480fd9p-1
   Maximum observed error in the special domain (|x| > 2^23)
   is 2.70 + 0.5ULP
   _ZGVnN2v_cos (0x1.0808d08f24a99p+854) got -0x1.fe675082631d2p-3
					want -0x1.fe675082631cfp-3.  */
float64x2_t VPCS_ATTR V_NAME_D1 (cos) (float64x2_t x)
{
  const struct data *d = ptr_barrier (&data);
  float64x2_t n, r, r2, r3, r4, t1, t2, t3, y;
  uint64x2_t odd, cmp;

  cmp = vcageq_f64 (x, d->range_val);

  /* n = rint((|x|+pi/2)/pi) - 0.5.  */
  n = vrndaq_f64 (vfmaq_f64 (v_f64 (0.5), x, d->inv_pi));
  odd = vshlq_n_u64 (vreinterpretq_u64_s64 (vcvtq_s64_f64 (n)), 63);
  n = vsubq_f64 (n, v_f64 (0.5f));

  /* r = |x| - n*pi  (range reduction into -pi/2 .. pi/2).  */
  r = vfmsq_f64 (x, d->pi_1, n);
  r = vfmsq_f64 (r, d->pi_2, n);
  r = vfmsq_f64 (r, d->pi_3, n);

  /* sin(r) poly approx.  */
  r2 = vmulq_f64 (r, r);
  r3 = vmulq_f64 (r2, r);
  r4 = vmulq_f64 (r2, r2);

  t1 = vfmaq_f64 (C (4), C (5), r2);
  t2 = vfmaq_f64 (C (2), C (3), r2);
  t3 = vfmaq_f64 (C (0), C (1), r2);

  y = vfmaq_f64 (t1, C (6), r4);
  y = vfmaq_f64 (t2, y, r4);
  y = vfmaq_f64 (t3, y, r4);
  y = vfmaq_f64 (r, y, r3);

  if (unlikely (v_any_u64 (cmp)))
    return special_case (x, y, odd, cmp);
  return vreinterpretq_f64_u64 (veorq_u64 (vreinterpretq_u64_f64 (y), odd));
}

TEST_SIG (V, D, 1, cos, -3.1, 3.1)
TEST_ULP (V_NAME_D1 (cos), 2.78)
TEST_SYM_INTERVAL (V_NAME_D1 (cos), 0, 0x1p23, 500000)
TEST_SYM_INTERVAL (V_NAME_D1 (cos), 0x1p23, inf, 10000)
