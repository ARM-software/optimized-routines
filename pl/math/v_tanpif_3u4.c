/*
 * Single-precision vector tanpif(x) function.
 *
 * Copyright (c) 2024, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "v_math.h"
#include "pl_sig.h"
#include "test_defs.h"
#include "poly_advsimd_f32.h"

const static struct v_tanpif_data
{
  float32x4_t c0, c2, c4, c6;
  float c1, c3, c5, c7;
} tanpif_data = {
  /* Coefficents for tan(pi * x).  */
  .c0 = V4 (0x1.921fb4p1f),  .c1 = 0x1.4abbcep3f,      .c2 = V4 (0x1.466b8p5f),
  .c3 = 0x1.461c72p7f,	     .c4 = V4 (0x1.42e9d4p9f), .c5 = 0x1.69e2c4p11f,
  .c6 = V4 (0x1.e85558p11f), .c7 = 0x1.a52e08p16f,
};

/* Approximation for single-precision vector tanpif(x)
   The maximum error is 3.34 ULP:
   _ZGVnN4v_tanpif(0x1.d6c09ap-2) got 0x1.f70aacp+2
				 want 0x1.f70aa6p+2.  */
float32x4_t VPCS_ATTR V_NAME_F1 (tanpi) (float32x4_t x)
{
  const struct v_tanpif_data *d = ptr_barrier (&tanpif_data);
  float32x4_t odd_coeffs = vld1q_f32 (&d->c1);
  float32x4_t rounded = vrndnq_f32 (x);

  // inf produces nan that propagates.
  float32x4_t x_reduced = vsubq_f32 (x, rounded);
  float32x4_t abs_x_reduced = vabdq_f32 (x, rounded);
  uint32x4_t should_flip = vcgtq_f32 (abs_x_reduced, v_f32 (0.25f));
  float32x4_t r_x = vbslq_f32 (
      should_flip, vsubq_f32 (v_f32 (0.5f), abs_x_reduced), abs_x_reduced);

  float32x4_t r_x2 = vmulq_f32 (r_x, r_x);
  float32x4_t r_x4 = vmulq_f32 (r_x2, r_x2);

  // pw_horner_7:
  float32x4_t p01 = vfmaq_laneq_f32 (d->c0, r_x2, odd_coeffs, 0);
  float32x4_t p23 = vfmaq_laneq_f32 (d->c2, r_x2, odd_coeffs, 1);
  float32x4_t p45 = vfmaq_laneq_f32 (d->c4, r_x2, odd_coeffs, 2);
  float32x4_t p67 = vfmaq_laneq_f32 (d->c6, r_x2, odd_coeffs, 3);
  float32x4_t p = vfmaq_f32 (p45, r_x4, p67);
  p = vfmaq_f32 (p23, r_x4, p);
  p = vfmaq_f32 (p01, r_x4, p);
  float32x4_t poly = vmulq_f32 (r_x, p);

  float32x4_t poly_recip = vdivq_f32 (v_f32 (1.0f), poly);
  float32x4_t result = vbslq_f32 (should_flip, poly_recip, poly);

  uint32x4_t sign = veorq_u32 (vreinterpretq_u32_f32 (x_reduced),
			       vreinterpretq_u32_f32 (abs_x_reduced));
  return vreinterpretq_f32_u32 (
      vorrq_u32 (vreinterpretq_u32_f32 (result), sign));
}

#if WANT_TRIGPI_TESTS
PL_SIG (V, F, 1, tanpi, -3.1, 3.1)
TEST_DISABLE_FENV (V_NAME_F1 (tanpi))
TEST_ULP (V_NAME_F1 (tanpi), 2.84)
TEST_SYM_INTERVAL (V_NAME_F1 (tanpi), 0, 0x1p-31, 50000)
TEST_SYM_INTERVAL (V_NAME_F1 (tanpi), 0x1p-31, 0.5, 100000)
TEST_SYM_INTERVAL (V_NAME_F1 (tanpi), 0.5, 0x1p23f, 100000)
TEST_SYM_INTERVAL (V_NAME_F1 (tanpi), 0x1p23f, inf, 100000)
#endif