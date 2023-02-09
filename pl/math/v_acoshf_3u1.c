/*
 * Single-precision vector acosh(x) function.
 * Copyright (c) 2023, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "v_math.h"
#include "pl_sig.h"
#include "pl_test.h"

#define SignMask 0x80000000
#define One 0x3f800000
#define SquareLim 0x5f800000 /* asuint(0x1p64).  */

#include "v_log1pf_inline.h"

static NOINLINE VPCS_ATTR float32x4_t
special_case (float32x4_t x, float32x4_t y, uint32x4_t special)
{
  return v_call_f32 (acoshf, x, y, special);
}

/* Vector approximation for single-precision acosh, based on log1p. Maximum
   error depends on WANT_SIMD_EXCEPT. With SIMD fp exceptions enabled, it
   is 2.78 ULP:
   __v_acoshf(0x1.07887p+0) got 0x1.ef9e9cp-3
			   want 0x1.ef9ea2p-3.
   With exceptions disabled, we can compute u with a shorter dependency chain,
   which gives maximum error of 3.07 ULP:
  __v_acoshf(0x1.01f83ep+0) got 0x1.fbc7fap-4
			   want 0x1.fbc7f4p-4.  */

VPCS_ATTR float32x4_t V_NAME_F1 (acosh) (float32x4_t x)
{
  uint32x4_t ix = v_as_u32_f32 (x);
  uint32x4_t special = v_cond_u32 ((ix - One) >= (SquareLim - One));

#if WANT_SIMD_EXCEPT
  /* Mask special lanes with 1 to side-step spurious invalid or overflow. Use
     only xm1 to calculate u, as operating on x will trigger invalid for NaN. */
  float32x4_t xm1 = vbslq_f32 (special, v_f32 (1), x - 1);
  float32x4_t u = v_fma_f32 (xm1, xm1, 2 * xm1);
#else
  float32x4_t xm1 = x - 1;
  float32x4_t u = xm1 * (x + 1.0f);
#endif
  float32x4_t y = log1pf_inline (xm1 + vsqrtq_f32 (u));

  if (unlikely (v_any_u32 (special)))
    return special_case (x, y, special);
  return y;
}

PL_SIG (V, F, 1, acosh, 1.0, 10.0)
#if WANT_SIMD_EXCEPT
PL_TEST_ULP (V_NAME_F1 (acosh), 2.29)
#else
PL_TEST_ULP (V_NAME_F1 (acosh), 2.58)
#endif
PL_TEST_EXPECT_FENV (V_NAME_F1 (acosh), WANT_SIMD_EXCEPT)
PL_TEST_INTERVAL (V_NAME_F1 (acosh), 0, 1, 500)
PL_TEST_INTERVAL (V_NAME_F1 (acosh), 1, SquareLim, 100000)
PL_TEST_INTERVAL (V_NAME_F1 (acosh), SquareLim, inf, 1000)
PL_TEST_INTERVAL (V_NAME_F1 (acosh), -0, -inf, 1000)
