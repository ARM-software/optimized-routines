/*
 * Single-precision vector sincos function.
 *
 * Copyright (c) 2023-2026, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "v_sincosf_common.h"
#include "v_math.h"
#include "test_defs.h"

static void VPCS_ATTR NOINLINE
special_case (float32x4_t x, uint32x4_t special, float *out_sin,
	      float *out_cos)
{
  const struct v_sincosf_data *d = ptr_barrier (&v_sincosf_data);
  uint32x4_t is_finite = vcaltq_f32 (x, v_f32 (INFINITY));
  special = vandq_u32 (special, is_finite);
  float32x4x2_t large = sincos_fallback (x);
  float32x4x2_t sc = v_sincosf_inline (x, d);
  float32x4_t sin = vbslq_f32 (special, large.val[0], sc.val[0]);
  float32x4_t cos = vbslq_f32 (special, large.val[1], sc.val[1]);

  vst1q_f32 (out_sin, sin);
  vst1q_f32 (out_cos, cos);
}

/* Single-precision vector function allowing calculation of both sin and cos in
   one function call, using shared argument reduction and separate low-order
   polynomials.
   The maximum observed error is 1.17 + 0.5 ULP for sin if |x| < 0x1p20.
   _ZGVnN4vl4l4_sincosf_sin (0x1.c704c4p+19)
    got 0x1.fff698p-5
   want 0x1.fff69cp-5
   The maximum observed error is 1.31 + 0.5ULP for cos if |x| < 0x1p20.
   _ZGVnN4vl4l4_sincosf_cos (0x1.e506fp+19)
    got -0x1.ffec6ep-6
   want -0x1.ffec72p-6.
   The special domain has a slightly higher maximum error than the fast path:
   Maximum observed error is 1.42 + 0.5ULP for sin if |x| >= 0x1p20.
   _ZGVnN4vl4l4_sincosf_sin (0x1.dbe63cp+36)
    got -0x1.d76b4p-2
   want -0x1.d76b44p-2.
   Maximum observed error is 1.43 + 0.5ULP for cos if |x| >= 0x1p20.
   _ZGVnN4vl4l4_sincosf_cos (0x1.a2785ap+108)
    got -0x1.dc17fp-2
   want -0x1.dc17f4p-2.  */
VPCS_ATTR void
_ZGVnN4vl4l4_sincosf (float32x4_t x, float *out_sin, float *out_cos)
{
  const struct v_sincosf_data *d = ptr_barrier (&v_sincosf_data);
  uint32x4_t special = check_ge_rangeval (x, d);

  if (unlikely (v_any_u32 (special)))
    return special_case (x, special, out_sin, out_cos);

  float32x4x2_t sc = v_sincosf_inline (x, d);

  vst1q_f32 (out_sin, sc.val[0]);
  vst1q_f32 (out_cos, sc.val[1]);
}

TEST_ULP (_ZGVnN4vl4l4_sincosf_sin, 1.42)
TEST_ULP (_ZGVnN4vl4l4_sincosf_cos, 1.43)
#define V_SINCOSF_INTERVAL(lo, hi, n)                                         \
  TEST_INTERVAL (_ZGVnN4vl4l4_sincosf_sin, lo, hi, n)                         \
  TEST_INTERVAL (_ZGVnN4vl4l4_sincosf_cos, lo, hi, n)
V_SINCOSF_INTERVAL (0, 0x1p-31, 50000)
V_SINCOSF_INTERVAL (0x1p-31, 0x1p20, 500000)
V_SINCOSF_INTERVAL (0x1p20, inf, 10000)
