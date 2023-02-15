/*
 * Single-precision vector atanh(x) function.
 *
 * Copyright (c) 2022-2023, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "v_math.h"
#include "mathlib.h"
#include "pl_sig.h"
#include "pl_test.h"

#include "v_log1pf_inline.h"

#define AbsMask 0x7fffffff
#define Half 0x3f000000
#define One 0x3f800000
#define TinyBound 0x39800000 /* 0x1p-12, below which atanhf(x) rounds to x. */

/* Approximation for vector single-precision atanh(x) using modified log1p.
   The maximum error is 3.08 ULP:
   __v_atanhf(0x1.ff215p-5) got 0x1.ffcb7cp-5
			   want 0x1.ffcb82p-5.  */
VPCS_ATTR float32x4_t V_NAME_F1 (atanh) (float32x4_t x)
{
  uint32x4_t ix = vreinterpretq_u32_f32 (x);
  float32x4_t halfsign
    = vreinterpretq_f32_u32 (vbslq_u32 (v_u32 (AbsMask), v_u32 (Half), ix));
  uint32x4_t iax = ix & AbsMask;

  float32x4_t ax = vreinterpretq_f32_u32 (iax);

#if WANT_SIMD_EXCEPT
  uint32x4_t special = (iax >= One) | (iax <= TinyBound);
  /* Side-step special cases by setting those lanes to 0, which will trigger no
     exceptions. These will be fixed up later.  */
  if (unlikely (v_any_u32 (special)))
    ax = vbslq_f32 (special, v_f32 (0), ax);
#else
  uint32x4_t special = iax >= One;
#endif

  float32x4_t y = halfsign * log1pf_inline ((2 * ax) / (1 - ax));

  if (unlikely (v_any_u32 (special)))
    return v_call_f32 (atanhf, x, y, special);
  return y;
}

PL_SIG (V, F, 1, atanh, -1.0, 1.0)
PL_TEST_ULP (V_NAME_F1 (atanh), 2.59)
PL_TEST_EXPECT_FENV (V_NAME_F1 (atanh), WANT_SIMD_EXCEPT)
PL_TEST_INTERVAL_C (V_NAME_F1 (atanh), 0, 0x1p-12, 500, 0)
PL_TEST_INTERVAL_C (V_NAME_F1 (atanh), 0x1p-12, 1, 200000, 0)
PL_TEST_INTERVAL_C (V_NAME_F1 (atanh), 1, inf, 1000, 0)
PL_TEST_INTERVAL_C (V_NAME_F1 (atanh), -0, -0x1p-12, 500, 0)
PL_TEST_INTERVAL_C (V_NAME_F1 (atanh), -0x1p-12, -1, 200000, 0)
PL_TEST_INTERVAL_C (V_NAME_F1 (atanh), -1, -inf, 1000, 0)
