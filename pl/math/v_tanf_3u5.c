/*
 * Single-precision vector tan(x) function.
 *
 * Copyright (c) 2021-2023, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "v_math.h"
#include "estrinf.h"
#include "pl_sig.h"
#include "pl_test.h"

struct v_tanf_data
{
  float32x4_t poly[6];
  float32x4_t neg_half_pi_1, neg_half_pi_2, neg_half_pi_3, two_over_pi, shift;
#if !WANT_SIMD_EXCEPT
  float32x4_t range_val;
#endif
};

static const volatile struct v_tanf_data data
  = {.neg_half_pi_1 = V4 (-0x1.921fb6p+0f),
     .neg_half_pi_2 = V4 (0x1.777a5cp-25f),
     .neg_half_pi_3 = V4 (0x1.ee59dap-50f),
     .two_over_pi = V4 (0x1.45f306p-1f),
     .shift = V4 (0x1.8p+23f),
#if !WANT_SIMD_EXCEPT
     .range_val = V4 (0x1p15f),
#endif
     .poly = {V4 (0x1.55555p-2f), V4 (0x1.11166p-3f), V4 (0x1.b88a78p-5f),
	      V4 (0x1.7b5756p-6f), V4 (0x1.4ef4cep-8f), V4 (0x1.0e1e74p-7f)}};

#define RangeVal v_u32 (0x47000000)  /* asuint32(0x1p15f).  */
#define TinyBound v_u32 (0x30000000) /* asuint32 (0x1p-31f).  */
#define Thresh v_u32 (0x16000000)    /* asuint32(RangeVal) - TinyBound.  */
#define C(i) data.poly[i]

/* Special cases (fall back to scalar calls).  */
static float32x4_t VPCS_ATTR NOINLINE
special_case (float32x4_t x, float32x4_t y, uint32x4_t cmp)
{
  return v_call_f32 (tanf, x, y, cmp);
}

/* Use a full Estrin scheme to evaluate polynomial.  */
static inline float32x4_t
eval_poly (float32x4_t z)
{
  float32x4_t z2 = vmulq_f32 (z, z);
#if WANT_SIMD_EXCEPT
  /* Tiny z (<= 0x1p-31) will underflow when calculating z^4. If fp exceptions
     are to be triggered correctly, sidestep this by fixing such lanes to 0.  */
  uint32x4_t will_uflow
    = vcleq_u32 (vreinterpretq_u32_f32 (vabsq_f32 (z)), TinyBound);
  if (unlikely (v_any_u32 (will_uflow)))
    z2 = vbslq_f32 (will_uflow, v_f32 (0), z2);
#endif
  float32x4_t z4 = vmulq_f32 (z2, z2);
  return ESTRIN_5 (z, z2, z4, C);
}

/* Fast implementation of AdvSIMD tanf.
   Maximum error is 3.45 ULP:
   __v_tanf(-0x1.e5f0cap+13) got 0x1.ff9856p-1
			    want 0x1.ff9850p-1.  */
float32x4_t VPCS_ATTR V_NAME_F1 (tan) (float32x4_t x)
{
  float32x4_t special_arg = x;

  /* iax >= RangeVal means x, if not inf or NaN, is too large to perform fast
     regression.  */
#if WANT_SIMD_EXCEPT
  uint32x4_t iax = vreinterpretq_u32_f32 (vabsq_f32 (x));
  /* If fp exceptions are to be triggered correctly, also special-case tiny
     input, as this will load to overflow later. Fix any special lanes to 1 to
     prevent any exceptions being triggered.  */
  uint32x4_t special = vcgeq_u32 (vsubq_u32 (iax, TinyBound), Thresh);
  if (unlikely (v_any_u32 (special)))
    x = vbslq_f32 (special, v_f32 (1.0f), x);
#else
  /* Otherwise, special-case large and special values.  */
  uint32x4_t special = vcageq_f32 (x, data.range_val);
#endif

  /* n = rint(x/(pi/2)).  */
  float32x4_t q = vfmaq_f32 (data.shift, data.two_over_pi, x);
  float32x4_t n = vsubq_f32 (q, data.shift);
  /* n is representable as a signed integer, simply convert it.  */
  int32x4_t in = vcvtaq_s32_f32 (n);
  /* Determine if x lives in an interval, where |tan(x)| grows to infinity.  */
  int32x4_t alt = vandq_s32 (in, v_s32 (1));
  /* alt != 0.  */
  uint32x4_t pred_alt = vmvnq_u32 (vceqzq_s32 (alt));

  /* r = x - n * (pi/2)  (range reduction into -pi./4 .. pi/4).  */
  float32x4_t r;
  r = vfmaq_f32 (x, data.neg_half_pi_1, n);
  r = vfmaq_f32 (r, data.neg_half_pi_2, n);
  r = vfmaq_f32 (r, data.neg_half_pi_3, n);

  /* If x lives in an interval, where |tan(x)|
     - is finite, then use a polynomial approximation of the form
       tan(r) ~ r + r^3 * P(r^2) = r + r * r^2 * P(r^2).
     - grows to infinity then use symmetries of tangent and the identity
       tan(r) = cotan(pi/2 - r) to express tan(x) as 1/tan(-r). Finally, use
       the same polynomial approximation of tan as above.  */

  /* Invert sign of r if odd quadrant.  */
  float32x4_t z = vmulq_f32 (r, vbslq_f32 (pred_alt, v_f32 (-1), v_f32 (1)));

  /* Evaluate polynomial approximation of tangent on [-pi/4, pi/4].  */
  float32x4_t z2 = vmulq_f32 (r, r);
  float32x4_t p = eval_poly (z2);
  float32x4_t y = vfmaq_f32 (z, vmulq_f32 (z, z2), p);

  /* Compute reciprocal and apply if required.  */
  float32x4_t inv_y = vdivq_f32 (v_f32 (1.0f), y);
  y = vbslq_f32 (pred_alt, inv_y, y);

  /* Fast reduction does not handle the x = -0.0 case well,
     therefore it is fixed here.  */
  y = vbslq_f32 (vceqzq_f32 (x), x, y);

  if (unlikely (v_any_u32 (special)))
    return special_case (special_arg, y, special);
  return y;
}

PL_SIG (V, F, 1, tan, -3.1, 3.1)
PL_TEST_ULP (V_NAME_F1 (tan), 2.96)
PL_TEST_EXPECT_FENV (V_NAME_F1 (tan), WANT_SIMD_EXCEPT)
PL_TEST_INTERVAL (V_NAME_F1 (tan), -0.0, -0x1p126, 100)
PL_TEST_INTERVAL (V_NAME_F1 (tan), 0x1p-149, 0x1p-126, 4000)
PL_TEST_INTERVAL (V_NAME_F1 (tan), 0x1p-126, 0x1p-23, 50000)
PL_TEST_INTERVAL (V_NAME_F1 (tan), 0x1p-23, 0.7, 50000)
PL_TEST_INTERVAL (V_NAME_F1 (tan), 0.7, 1.5, 50000)
PL_TEST_INTERVAL (V_NAME_F1 (tan), 1.5, 100, 50000)
PL_TEST_INTERVAL (V_NAME_F1 (tan), 100, 0x1p17, 50000)
PL_TEST_INTERVAL (V_NAME_F1 (tan), 0x1p17, inf, 50000)
