/*
 * Single-precision vector cosh(x) function.
 *
 * Copyright (c) 2022-2025, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "v_expf_inline.h"
#include "v_math.h"
#include "test_sig.h"
#include "test_defs.h"

static const struct data
{
  struct v_expf_data expf_consts;
  float32x4_t bound;
} data = {
  .expf_consts = V_EXPF_DATA,
  /* 0x1.5a92d8p+6: expf overflows above this, so have to use special case.  */
  .bound = V4 (0x1.5a92d8p+6),
};

static float32x4_t NOINLINE VPCS_ATTR
special_case (float32x4_t x, float32x4_t half_t, float32x4_t half_over_t,
	      uint32x4_t special)
{
  return v_call_f32 (coshf, x, vaddq_f32 (half_t, half_over_t), special);
}

/* Single-precision vector cosh, using vector expf.
   Maximum error is 2.38 ULP:
   _ZGVnN4v_coshf (0x1.e8001ep+1) got 0x1.6a491ep+4
				 want 0x1.6a4922p+4.  */
float32x4_t VPCS_ATTR NOINLINE V_NAME_F1 (cosh) (float32x4_t x)
{
  const struct data *d = ptr_barrier (&data);

  uint32x4_t special = vcageq_f32 (x, d->bound);
  float32x4_t t = v_expf_inline (x, &d->expf_consts);

  /* Calculate cosh by exp(x) / 2 + exp(-x) / 2.  */
  float32x4_t half_t = vmulq_n_f32 (t, 0.5);
  float32x4_t half_over_t = vdivq_f32 (v_f32 (0.5), t);

  if (unlikely (v_any_u32 (special)))
    return special_case (x, half_t, half_over_t, special);

  return vaddq_f32 (half_t, half_over_t);
}

HALF_WIDTH_ALIAS_F1 (cosh)

TEST_SIG (V, F, 1, cosh, -10.0, 10.0)
TEST_ULP (V_NAME_F1 (cosh), 1.89)
TEST_SYM_INTERVAL (V_NAME_F1 (cosh), 0, 0x1p-63, 100)
TEST_SYM_INTERVAL (V_NAME_F1 (cosh), 0x1p-63, 1, 1000)
TEST_SYM_INTERVAL (V_NAME_F1 (cosh), 1, 0x1.5a92d8p+6, 80000)
TEST_SYM_INTERVAL (V_NAME_F1 (cosh), 0x1.5a92d8p+6, inf, 2000)
