/*
 * Single-precision vector cosh(x) function.
 *
 * Copyright (c) 2022-2023, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "v_math.h"
#include "mathlib.h"
#include "pl_sig.h"
#include "pl_test.h"

#define AbsMask 0x7fffffff
#define TinyBound 0x20000000 /* 0x1p-63: Round to 1 below this.  */
#define SpecialBound                                                           \
  0x42ad496c /* 0x1.5a92d8p+6: expf overflows above this, so have to use       \
		special case.  */
#define Half v_f32 (0.5)

float32x4_t __v_expf (float32x4_t);

/* Single-precision vector cosh, using vector expf.
   Maximum error is 2.38 ULP:
   __v_coshf(0x1.e8001ep+1) got 0x1.6a491ep+4 want 0x1.6a4922p+4.  */
VPCS_ATTR float32x4_t V_NAME_F1 (cosh) (float32x4_t x)
{
  uint32x4_t ix = vreinterpretq_u32_f32 (x);
  uint32x4_t iax = ix & AbsMask;
  float32x4_t ax = vreinterpretq_f32_u32 (iax);
  uint32x4_t special = iax >= SpecialBound;

#if WANT_SIMD_EXCEPT
  /* If fp exceptions are to be triggered correctly, fall back to the scalar
     variant for all inputs if any input is a special value or above the bound
     at which expf overflows. */
  if (unlikely (v_any_u32 (special)))
    return v_call_f32 (coshf, x, x, v_u32 (-1));

  uint32x4_t tiny = iax <= TinyBound;
  /* If any input is tiny, avoid underflow exception by fixing tiny lanes of
     input to 1, which will generate no exceptions, and then also fixing tiny
     lanes of output to 1 just before return.  */
  if (unlikely (v_any_u32 (tiny)))
    ax = vbslq_f32 (tiny, v_f32 (1), ax);
#endif

  /* Calculate cosh by exp(x) / 2 + exp(-x) / 2.  */
  float32x4_t t = __v_expf (ax);
  float32x4_t y = t * Half + Half / t;

#if WANT_SIMD_EXCEPT
  if (unlikely (v_any_u32 (tiny)))
    return vbslq_f32 (tiny, v_f32 (1), y);
#else
  if (unlikely (v_any_u32 (special)))
    return v_call_f32 (coshf, x, y, special);
#endif

  return y;
}

PL_SIG (V, F, 1, cosh, -10.0, 10.0)
PL_TEST_ULP (V_NAME_F1 (cosh), 1.89)
PL_TEST_EXPECT_FENV (V_NAME_F1 (cosh), WANT_SIMD_EXCEPT)
PL_TEST_INTERVAL (V_NAME_F1 (cosh), 0, 0x1p-63, 100)
PL_TEST_INTERVAL (V_NAME_F1 (cosh), 0, 0x1.5a92d8p+6, 80000)
PL_TEST_INTERVAL (V_NAME_F1 (cosh), 0x1.5a92d8p+6, inf, 2000)
PL_TEST_INTERVAL (V_NAME_F1 (cosh), -0, -0x1p-63, 100)
PL_TEST_INTERVAL (V_NAME_F1 (cosh), -0, -0x1.5a92d8p+6, 80000)
PL_TEST_INTERVAL (V_NAME_F1 (cosh), -0x1.5a92d8p+6, -inf, 2000)
