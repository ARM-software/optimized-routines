/*
 * Low-accuracy single-precision vector e^x function.
 *
 * Copyright (c) 2025, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "sv_math.h"
#include "test_sig.h"
#include "test_defs.h"
#include "sv_expf_inline.h"

/* Roughly 87.3. For x < -Thres, the result is subnormal and not handled
   correctly by FEXPA.  */
#define Thres 0x1.5d5e2ap+6f

static const struct data
{
  struct sv_expf_data d;
  float thres;
} data = {
  .d = SV_EXPF_DATA,
  .thres = Thres,
};

static svfloat32_t NOINLINE
special_case (svfloat32_t x, svbool_t special, const struct sv_expf_data *d)
{
  return sv_call_f32 (expf, x, expf_inline (x, svptrue_b32 (), d), special);
}

/* Optimised single-precision SVE exp function.
   Worst-case error is 245.5 +0.5 ULP.
   arm_math_sve_fast_expf(0x1.365b4ep+3) got 0x1.fd3a4p+13
					want 0x1.fd3c2cp+13.  */
svfloat32_t
arm_math_sve_fast_expf (svfloat32_t x, const svbool_t pg)
{
  const struct data *d = ptr_barrier (&data);
  svbool_t is_special_case = svacgt (pg, x, d->thres);
  if (unlikely (svptest_any (pg, is_special_case)))
    return special_case (x, is_special_case, &d->d);
  return expf_inline (x, pg, &d->d);
}

TEST_ULP (arm_math_sve_fast_expf, 4096)
TEST_DISABLE_FENV (arm_math_sve_fast_expf)
TEST_SYM_INTERVAL (arm_math_sve_fast_expf, 0, Thres, 50000)
TEST_SYM_INTERVAL (arm_math_sve_fast_expf, Thres, inf, 50000)
CLOSE_SVE_ATTR
