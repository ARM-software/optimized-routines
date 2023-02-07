/*
 * Single-precision vector tanh(x) function.
 *
 * Copyright (c) 2022-2023, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "v_math.h"
#include "pl_sig.h"
#include "pl_test.h"

#include "v_expm1f_inline.h"

#define BoringBound                                                            \
  0x41102cb3 /* 0x1.205966p+3, above which tanhf rounds to 1 (or -1 for        \
		negative).  */
#define AbsMask 0x7fffffff

static NOINLINE float32x4_t
special_case (float32x4_t x, float32x4_t y, uint32x4_t special)
{
  return v_call_f32 (tanhf, x, y, special);
}

/* Approximation for single-precision vector tanh(x), using a simplified version
   of expm1f. The maximum error is 2.58 ULP:
   __v_tanhf(0x1.fa5eep-5) got 0x1.f9ba02p-5
			  want 0x1.f9ba08p-5.  */
VPCS_ATTR float32x4_t V_NAME_F1 (tanh) (float32x4_t x)
{
  uint32x4_t ix = v_as_u32_f32 (x);
  uint32x4_t iax = ix & AbsMask;
  uint32x4_t sign = ix & ~AbsMask;
  uint32x4_t is_boring = v_cond_u32 (iax > BoringBound);
  float32x4_t boring = v_as_f32_u32 (sign | One);

#if WANT_SIMD_EXCEPT
  /* If fp exceptions are to be triggered properly, set all special and boring
     lanes to 1, which will trigger no exceptions, and fix them up later.  */
  uint32x4_t special = v_cond_u32 ((iax > 0x7f800000) | (iax < 0x34000000));
  ix = v_sel_u32 (is_boring, v_u32 (One), ix);
  if (unlikely (v_any_u32 (special)))
    ix = v_sel_u32 (special, v_u32 (One), ix);
#else
  uint32x4_t special = v_cond_u32 ((iax > 0x7f800000) | (iax == 0));
#endif

  /* tanh(x) = (e^2x - 1) / (e^2x + 1).  */
  float32x4_t q = expm1f_inline (2 * v_as_f32_u32 (ix));
  float32x4_t y = q / (q + 2);
  y = v_sel_f32 (is_boring, boring, y);
  if (unlikely (v_any_u32 (special)))
    return special_case (x, y, special);
  return y;
}

PL_SIG (V, F, 1, tanh, -10.0, 10.0)
PL_TEST_ULP (V_NAME_F1 (tanh), 2.09)
PL_TEST_EXPECT_FENV (V_NAME_F1 (tanh), WANT_SIMD_EXCEPT)
PL_TEST_INTERVAL (V_NAME_F1 (tanh), 0, 0x1p-23, 1000)
PL_TEST_INTERVAL (V_NAME_F1 (tanh), -0, -0x1p-23, 1000)
PL_TEST_INTERVAL (V_NAME_F1 (tanh), 0x1p-23, 0x1.205966p+3, 100000)
PL_TEST_INTERVAL (V_NAME_F1 (tanh), -0x1p-23, -0x1.205966p+3, 100000)
PL_TEST_INTERVAL (V_NAME_F1 (tanh), 0x1.205966p+3, inf, 100)
PL_TEST_INTERVAL (V_NAME_F1 (tanh), -0x1.205966p+3, -inf, 100)
