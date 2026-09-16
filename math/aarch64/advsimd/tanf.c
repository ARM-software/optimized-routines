/*
 * Single-precision vector tan(x) function.
 *
 * Copyright (c) 2021-2026, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "v_math.h"
#include "v_poly_f32.h"
#include "test_sig.h"
#include "test_defs.h"
#include "v_sincosf_common.h"

static const struct data
{
  float32x4_t c0, c2, c4;
  float c1, c3, c5;
  float inv_pio2, pio2_1, pio2_2, pio2_3;
  float32x4_t shift;
  float32x4_t range_val;
} data = {
  /* Coefficients generated using FPMinimax.  */
  .c0 = V4 (0x1.55555p-2f),
  .c1 = 0x1.11166p-3f,
  .c2 = V4 (0x1.b88a78p-5f),
  .c3 = 0x1.7b5756p-6f,
  .c4 = V4 (0x1.4ef4cep-8f),
  .c5 = 0x1.0e1e74p-7f,
  /* Stores constants: (-pi/2)_high, (-pi/2)_mid, (-pi/2)_low, and 2/pi.  */
  .inv_pio2 = 0x1.45f306p-1f,
  .pio2_1 = -0x1.921fb6p+0f,
  .pio2_2 = 0x1.777a5cp-25f,
  .pio2_3 = 0x1.ee59dap-50f,
  .shift = V4 (0x1.8p+23f),
  .range_val = V4 (0x1p15f),
};

static float32x4_t VPCS_ATTR NOINLINE
special_case (float32x4_t x, float32x4_t y, uint32x4_t pred_alt,
	      uint32x4_t special)
{
  uint32x4_t is_inf = vcageq_f32 (x, v_f32 (INFINITY));

  /* For large values, we can compute tan(x) = sin(x) / cos(x).  */
  float32x4x2_t sc = sincos_fallback (x);

  /* For special lanes, computes sin(x)/cos(x)
     For non-special lanes, computes 1/y.  */
  float32x4_t n = vbslq_f32 (special, sc.val[0], v_f32 (1.0f));
  float32x4_t d = vbslq_f32 (special, sc.val[1], y);

  float32x4_t div = vdivq_f32 (n, d);

  /* Swap y back into non-special lanes where pred_alt is false.  */
  uint32x4_t swap = vorrq_u32 (pred_alt, special);
  float32x4_t ret = vbslq_f32 (swap, div, y);

  /* Handle inf case.  */
  return vbslq_f32 (is_inf, v_f32 (NAN), ret);
}
/* Vector version of tanf.
   Maximum observed error is 2.95 + 0.5 ULP if |x| < 0x1p15.
   _ZGVnN4v_tanf (0x1.e5f0cap+13)
    got -0x1.ff9856p-1
   want -0x1.ff985p-1.
   The special domain has a lower maximum error than the fast path:
   Maximum observed error is 2.57 + 0.5ULP
   _ZGVnN4v_tanf (0x1.9d6e32p+39)
    got 0x1.ba648p+0
   want 0x1.ba647ap+0.  */
float32x4_t VPCS_ATTR NOINLINE V_NAME_F1 (tan) (float32x4_t x)
{
  const struct data *d = ptr_barrier (&data);
  float32x4_t special_arg = x;

  /* Special-case large and special values.  */
  uint32x4_t special = vcageq_f32 (x, d->range_val);

  /* n = rint(x/(pi/2)).  */
  float32x4_t pi_consts = vld1q_f32 (&d->inv_pio2);
  float32x4_t q = vfmaq_laneq_f32 (d->shift, x, pi_consts, 0);
  float32x4_t n = vsubq_f32 (q, d->shift);
  /* Determine if x lives in an interval, where |tan(x)| grows to infinity.  */
  uint32x4_t pred_alt = vtstq_u32 (vreinterpretq_u32_f32 (q), v_u32 (1));

  /* r = x - n * (pi/2)  (range reduction into -pi./4 .. pi/4).  */
  float32x4_t r;
  r = vfmaq_laneq_f32 (x, n, pi_consts, 1);
  r = vfmaq_laneq_f32 (r, n, pi_consts, 2);
  r = vfmaq_laneq_f32 (r, n, pi_consts, 3);

  /* If x lives in an interval, where |tan(x)|
     - is finite, then use a polynomial approximation of the form
       tan(r) ~ r + r^3 * P(r^2) = r + r * r^2 * P(r^2).
     - grows to infinity then use symmetries of tangent and the identity
       tan(r) = cotan(pi/2 - r) to express tan(x) as 1/tan(-r). Finally, use
       the same polynomial approximation of tan as above.  */

  /* Invert sign of r if odd quadrant.  */
  float32x4_t z = vmulq_f32 (r, vbslq_f32 (pred_alt, v_f32 (-1), v_f32 (1)));

  /* Evaluate polynomial approximation of tangent on [-pi/4, pi/4].  */
  float32x4_t r2 = vmulq_f32 (r, r);
  float32x4_t r3 = vmulq_f32 (r2, z);
  float32x4_t r4 = vmulq_f32 (r2, r2);
  float32x4_t r8 = vmulq_f32 (r4, r4);

  float32x4_t coeffs = vld1q_f32 (&d->c1);
  float32x4_t p01 = vfmaq_laneq_f32 (d->c0, r2, coeffs, 0);
  float32x4_t p23 = vfmaq_laneq_f32 (d->c2, r2, coeffs, 1);
  float32x4_t p45 = vfmaq_laneq_f32 (d->c4, r2, coeffs, 2);

  float32x4_t p03 = vfmaq_f32 (p01, r4, p23);
  float32x4_t p = vfmaq_f32 (p03, r8, p45);

  float32x4_t y = vfmaq_f32 (z, r3, p);

  if (unlikely (v_any_u32 (special)))
    return special_case (special_arg, y, pred_alt, special);

  /* Compute reciprocal and apply if required.  */
  float32x4_t inv_y = vdivq_f32 (v_f32 (1.0f), y);

  return vbslq_f32 (pred_alt, inv_y, y);
}

HALF_WIDTH_ALIAS_F1 (tan)

TEST_SIG (V, F, 1, tan, -3.1, 3.1)
TEST_ULP (V_NAME_F1 (tan), 2.96)
TEST_SYM_INTERVAL (V_NAME_F1 (tan), 0, 0x1p-31, 5000)
TEST_SYM_INTERVAL (V_NAME_F1 (tan), 0x1p-31, 0x1p15, 500000)
TEST_SYM_INTERVAL (V_NAME_F1 (tan), 0x1p15, inf, 5000)
