/*
 * Single-precision vector cexpi function.
 *
 * Copyright (c) 2023-2026, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "v_sincosf_common.h"
#include "v_math.h"
#include "test_defs.h"

static float32x4x2_t VPCS_ATTR NOINLINE
special_case (float32x4_t x, uint32x4_t special)
{
  const struct v_sincosf_data *d = ptr_barrier (&v_sincosf_data);
  uint32x4_t is_finite = vcaltq_f32 (x, v_f32 (INFINITY));
  special = vandq_u32 (special, is_finite);
  float32x4x2_t large = sincos_fallback (x);
  float32x4x2_t y = v_sincosf_inline (x, d);
  y.val[0] = vbslq_f32 (special, large.val[0], y.val[0]);
  y.val[1] = vbslq_f32 (special, large.val[1], y.val[1]);
  return y;
}

/* Single-precision vector function allowing calculation of both sin and cos in
   one function call, using shared argument reduction and separate low-order
   polynomials.
   The maximum observed error is 1.17 + 0.5 ULP for sin if |x| < 0x1p20.
   _ZGVnN4v_cexpif_sin (0x1.c704c4p+19)
    got 0x1.fff698p-5
   want 0x1.fff69cp-5
   The maximum observed error is 1.31 + 0.5ULP for cos if |x| < 0x1p20.
   _ZGVnN4v_cexpif_cos (0x1.e506fp+19)
    got -0x1.ffec6ep-6
   want -0x1.ffec72p-6.
   The special domain has a slightly higher maximum error than the fast path:
   Maximum observed error is 1.42 + 0.5ULP for sin if |x| >= 0x1p20.
   _ZGVnN4v_cexpif_sin (0x1.dbe63cp+36)
    got -0x1.d76b4p-2
   want -0x1.d76b44p-2.
   Maximum observed error is 1.43 + 0.5ULP for cos if |x| >= 0x1p20.
   _ZGVnN4v_cexpif_cos (0x1.a2785ap+108)
    got -0x1.dc17fp-2
   want -0x1.dc17f4p-2.  */
VPCS_ATTR float32x4x2_t
_ZGVnN4v_cexpif (float32x4_t x)
{
  const struct v_sincosf_data *d = ptr_barrier (&v_sincosf_data);
  uint32x4_t special = check_ge_rangeval (x, d);
  if (unlikely (v_any_u32 (special)))
    return special_case (x, special);
  return v_sincosf_inline (x, d);
}

TEST_ULP (_ZGVnN4v_cexpif_sin, 1.42)
TEST_ULP (_ZGVnN4v_cexpif_cos, 1.43)
#define V_CEXPIF_INTERVAL(lo, hi, n)                                          \
  TEST_INTERVAL (_ZGVnN4v_cexpif_sin, lo, hi, n)                              \
  TEST_INTERVAL (_ZGVnN4v_cexpif_cos, lo, hi, n)
V_CEXPIF_INTERVAL (0, 0x1p20, 500000)
V_CEXPIF_INTERVAL (-0, -0x1p20, 500000)
V_CEXPIF_INTERVAL (0x1p20, inf, 10000)
V_CEXPIF_INTERVAL (-0x1p20, -inf, 10000)
