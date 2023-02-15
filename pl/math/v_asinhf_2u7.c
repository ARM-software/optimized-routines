/*
 * Single-precision vector asinh(x) function.
 *
 * Copyright (c) 2022-2023, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "v_math.h"
#include "include/mathlib.h"
#include "pl_sig.h"
#include "pl_test.h"

#define SignMask v_u32 (0x80000000)
#define One v_f32 (1.0f)
#define BigBound v_u32 (0x5f800000)  /* asuint(0x1p64).  */
#define TinyBound v_u32 (0x30800000) /* asuint(0x1p-30).  */

#include "v_log1pf_inline.h"

static NOINLINE float32x4_t
specialcase (float32x4_t x, float32x4_t y, uint32x4_t special)
{
  return v_call_f32 (asinhf, x, y, special);
}

/* Single-precision implementation of vector asinh(x), using vector log1p.
   Worst-case error is 2.66 ULP, at roughly +/-0.25:
   __v_asinhf(0x1.01b04p-2) got 0x1.fe163ep-3 want 0x1.fe1638p-3.  */
VPCS_ATTR float32x4_t V_NAME_F1 (asinh) (float32x4_t x)
{
  uint32x4_t ix = vreinterpretq_u32_f32 (x);
  uint32x4_t iax = ix & ~SignMask;
  uint32x4_t sign = ix & SignMask;
  float32x4_t ax = vreinterpretq_f32_u32 (iax);
  uint32x4_t special = iax >= BigBound;

#if WANT_SIMD_EXCEPT
  /* Sidestep tiny and large values to avoid inadvertently triggering
     under/overflow.  */
  special |= iax < TinyBound;
  if (unlikely (v_any_u32 (special)))
    ax = vbslq_f32 (special, One, ax);
#endif

  /* asinh(x) = log(x + sqrt(x * x + 1)).
     For positive x, asinh(x) = log1p(x + x * x / (1 + sqrt(x * x + 1))).  */
  float32x4_t d = One + vsqrtq_f32 (ax * ax + One);
  float32x4_t y = log1pf_inline (ax + ax * ax / d);
  y = vreinterpretq_f32_u32 (sign | vreinterpretq_u32_f32 (y));

  if (unlikely (v_any_u32 (special)))
    return specialcase (x, y, special);
  return y;
}

PL_SIG (V, F, 1, asinh, -10.0, 10.0)
PL_TEST_ULP (V_NAME_F1 (asinh), 2.17)
PL_TEST_EXPECT_FENV (V_NAME_F1 (asinh), WANT_SIMD_EXCEPT)
PL_TEST_INTERVAL (V_NAME_F1 (asinh), 0, 0x1p-12, 40000)
PL_TEST_INTERVAL (V_NAME_F1 (asinh), 0x1p-12, 1.0, 40000)
PL_TEST_INTERVAL (V_NAME_F1 (asinh), 1.0, 0x1p11, 40000)
PL_TEST_INTERVAL (V_NAME_F1 (asinh), 0x1p11, inf, 40000)
PL_TEST_INTERVAL (V_NAME_F1 (asinh), 0, -0x1p-12, 20000)
PL_TEST_INTERVAL (V_NAME_F1 (asinh), -0x1p-12, -1.0, 20000)
PL_TEST_INTERVAL (V_NAME_F1 (asinh), -1.0, -0x1p11, 20000)
PL_TEST_INTERVAL (V_NAME_F1 (asinh), -0x1p11, -inf, 20000)
