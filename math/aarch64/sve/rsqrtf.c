/*
 * Single-precision SVE rsqrt(x) function.
 *
 * Copyright (c) 2025, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "sv_math.h"
#include "test_defs.h"

static const struct data
{
  float32_t special_bound;
  int32_t scale_up, scale_down;
} data = {
  /* When x < 0x1p-128, estimate becomes infinity.
    x is scaled up by 0x1p22f, so estimate does not reach infinity.
    Then the result is multiplied by 0x1p11f.
    The difference between the lowest power possible (-149) and the special
    bound (-128) is 21. 22 is used here so that a power of 2 can be used for
    scaling in both directions.  */
  .special_bound = 0x1p-128f,
  .scale_up = 22,
  .scale_down = 11,
};

static inline svfloat32_t
inline_rsqrt (svfloat32_t x)
{
  /* Do estimate instruction.  */
  svfloat32_t estimate = svrsqrte_f32 (x);

  /* Do first step instruction.  */
  svfloat32_t estimate_squared = svmul_x (svptrue_b32 (), estimate, estimate);
  svfloat32_t step = svrsqrts_f32 (x, estimate_squared);
  estimate = svmul_x (svptrue_b32 (), estimate, step);

  /* Do second step instruction.
    This is required to achieve < 3.0 ULP.  */
  estimate_squared = svmul_x (svptrue_b32 (), estimate, estimate);
  step = svrsqrts_f32 (x, estimate_squared);
  estimate = svmul_x (svptrue_b32 (), estimate, step);
  return estimate;
}

static svfloat32_t NOINLINE
special_case (svfloat32_t x, svbool_t special, const struct data *d)
{
  x = svscale_f32_m (special, x, sv_s32 (d->scale_up));
  svfloat32_t estimate = inline_rsqrt (x);
  return svscale_f32_m (special, estimate, sv_s32 (d->scale_down));
}

/* Single-precision SVE implementation of rsqrtf(x).
  Maximum observed error: 1.47 + 0.5
  _ZGVsMxv_rsqrtf (0x1.f610dep+127) got 0x1.02852cp-64
				   want 0x1.02853p-64.  */
svfloat32_t SV_NAME_F1 (rsqrt) (svfloat32_t x, svbool_t pg)
{
  const struct data *d = ptr_barrier (&data);

  svbool_t special = svcmplt_n_f32 (pg, x, 0x1p-128f);
  if (unlikely (svptest_any (pg, special)))
    {
      return special_case (x, special, d);
    }
  return inline_rsqrt (x);
}

#if WANT_C23_TESTS
TEST_ULP (SV_NAME_F1 (rsqrt), 1.97)
TEST_SYM_INTERVAL (SV_NAME_F1 (rsqrt), 0, 1, 5000)
TEST_SYM_INTERVAL (SV_NAME_F1 (rsqrt), 1, 100, 5000)
TEST_INTERVAL (SV_NAME_F1 (rsqrt), 0, 0x1p-128f, 5000)
TEST_INTERVAL (SV_NAME_F1 (rsqrt), 100, inf, 5000)
TEST_INTERVAL (SV_NAME_F1 (rsqrt), -100, -inf, 5000)
#endif
CLOSE_SVE_ATTR