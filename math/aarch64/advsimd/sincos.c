/*
 * Double-precision vector sincos function.
 *
 * Copyright (c) 2023-2026, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "v_math.h"
#include "test_defs.h"
#include "v_sincos_common.h"

static void VPCS_ATTR NOINLINE
special_case (float64x2_t x, uint64x2_t special, double *out_sin,
	      double *out_cos)
{
  const struct v_sincos_data *d = ptr_barrier (&v_sincos_data);

  float64x2x2_t fast = v_sincos_inline (x, d);
  float64x2x2_t slow = v_sincos_fallback (x);

  /* Inf cases are handled correctly by the fast path, and incorrectly
     by the slow path. However, it's less costly to the fast path to
     handle them separately. So we do want to branch here for inf cases,
     but then use the fast path value anyway.  */
  uint64x2_t is_finite = vcaltq_f64 (x, v_f64 (INFINITY));
  special = vandq_u64 (special, is_finite);

  float64x2_t sin = vbslq_f64 (special, slow.val[0], fast.val[0]);
  float64x2_t cos = vbslq_f64 (special, slow.val[1], fast.val[1]);

  vst1q_f64 (out_sin, sin);
  vst1q_f64 (out_cos, cos);
}

/* Double-precision vector function allowing calculation of both sin and cos in
   one function call, using shared argument reduction and addition angle trig
   identities.
   Worst-case error for sin is 2.15 + 0.5 ULP when |x| >= 0x1p23:
   _ZGVnN2vl8l8_sincos_sin (0x1.3d4ded894041ep+784)
    got -0x1.fffa6b28930b5p-7
   want -0x1.fffa6b28930b2p-7
   Worst-case error for cos is 2.44 + 0.5 ULP when |x| >= 0x1p23:
   _ZGVnN2vl8l8_sincos_cos (0x1.aac6f8bffec82p+206)
    got -0x1.98ecd0b3020bfp-7
   want -0x1.98ecd0b3020bcp-7.

   Worst-case error for sin is 2.67 + 0.5 ULP when |x| < 0x1p23:
   _ZGVnN2vl8l8_sincos_sin (0x1.022ae05e6dae2p+12)
    got 0x1.f7f7190cb05e9p-2
   want 0x1.f7f7190cb05e6p-2
   Worst-case error for cos is 2.68 + 0.5 ULP when |x| < 0x1p23:
   _ZGVnN2vl8l8_sincos_cos (0x1.d187aa5fac8f7p+3)
    got -0x1.98c7cd085e031p-2
   want -0x1.98c7cd085e02ep-2.  */
VPCS_ATTR void
_ZGVnN2vl8l8_sincos (float64x2_t x, double *out_sin, double *out_cos)
{
  const struct v_sincos_data *d = ptr_barrier (&v_sincos_data);
  uint64x2_t special = vcagtq_f64 (x, d->range_val);

  if (unlikely (v_any_u64 (special)))
    return special_case (x, special, out_sin, out_cos);

  float64x2x2_t sc = v_sincos_inline (x, d);

  vst1q_f64 (out_sin, sc.val[0]);
  vst1q_f64 (out_cos, sc.val[1]);
}

TEST_ULP (_ZGVnN2vl8l8_sincos_sin, 2.68)
TEST_ULP (_ZGVnN2vl8l8_sincos_cos, 2.69)
#define V_SINCOS_INTERVAL(lo, hi, n)                                          \
  TEST_INTERVAL (_ZGVnN2vl8l8_sincos_sin, lo, hi, n)                          \
  TEST_INTERVAL (_ZGVnN2vl8l8_sincos_cos, lo, hi, n)
V_SINCOS_INTERVAL (0, 0x1p-31, 50000)
V_SINCOS_INTERVAL (0x1p-31, 0x1p23, 500000)
V_SINCOS_INTERVAL (0x1p23, inf, 10000)
