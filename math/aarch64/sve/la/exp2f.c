/*
 * Single-precision SVE 2^x function.
 *
 * Copyright (c) 2023-2026, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "sv_math.h"
#include "test_sig.h"
#include "test_defs.h"

/* For x < -SpecialBound, the result is subnormal and not handled
   correctly by FEXPA.  */
#define SpecialBound 0x1.f8p6f /* log2(2^126) = 126.0f.  */

/* Values of x over which exp overflows or underflows.  */
#define InfBound 0x1.0p7f    /* 128.0f.  */
#define ZeroBound -0x1.2cp7f /* -150.0f.  */

static const struct data
{
  float c0, c1, shift, special_bound;
  float inf_bound, zero_bound;
} data = {
  /* Coefficients generated using Remez algorithm with minimisation of relative
     error.  */
  .c0 = 0x1.62e485p-1,
  .c1 = 0x1.ebfbe0p-3,
  /* 1.5*2^17 + 127.  */
  .shift = 0x1.803f8p17f,
  .special_bound = SpecialBound,
  .inf_bound = InfBound,
  .zero_bound = ZeroBound,
};

static inline svfloat32_t
sv_exp2f_inline (svfloat32_t x, const svbool_t pg, const struct data *d)
{
  /* exp2(x) =
      2^n (1 + r * poly(r)), with 1 + r * poly(r) in [1/sqrt(2),sqrt(2)]
     x = n + r, with r in [-1/2, 1/2].  */
  svfloat32_t z = svadd_x (svptrue_b32 (), x, d->shift);
  svfloat32_t n = svsub_x (svptrue_b32 (), z, d->shift);
  svfloat32_t r = svsub_x (svptrue_b32 (), x, n);

  svfloat32_t scale = svexpa (svreinterpret_u32 (z));

  svfloat32_t poly = svmla_x (pg, sv_f32 (d->c0), r, sv_f32 (d->c1));
  poly = svmul_x (svptrue_b32 (), poly, r);

  return svmla_x (pg, scale, scale, poly);
}

/* Special case to handle large magnitudes of x, where fexpa returns nan.
   By applying a fixed offset to the value passed to fexpa,
   and then reapplying a scaling factor afterwards, this is avoided.  */
static svfloat32_t NOINLINE
special_case (svfloat32_t x, const svbool_t pg, const struct data *d)
{
  /* Computes the offset and scale factor based on sign of the input.  */
  svbool_t is_negative = svcmplt (pg, x, 0.0f);
  svfloat32_t offset = svneg_m (sv_f32 (23.0f), is_negative, sv_f32 (23.0f));
  svint32_t scale_adjust = svneg_m (sv_s32 (23), is_negative, sv_s32 (23));

  /* Bounds x between zero_bound and inf_bound, where exp2(x) would return
     0 or inf. By clamping x to these bounds, the behaviour of large values
     is more predictable.  */
  x = svmin_x (pg, svmax_x (pg, x, d->zero_bound), d->inf_bound);

  /* exp2(x) =
      2^n (1 + r * poly(r)), with 1 + r * poly(r) in [1/sqrt(2),sqrt(2)]
     x = n + r, with r in [-1/2, 1/2].  */
  svfloat32_t z = svadd_x (svptrue_b32 (), x, d->shift);
  svfloat32_t n = svsub_x (svptrue_b32 (), z, d->shift);

  svfloat32_t r = svsub_x (svptrue_b32 (), x, n);

  /* Computes scale with an offset, which returns:
     scale = 2^(n/N - 23).  */
  z = svsub_x (pg, z, offset);
  svfloat32_t scale = svexpa (svreinterpret_u32 (z));

  /* poly(r) = (exp2(r) - 1) / r = log(2) + r*log(2)^2/2.  */
  svfloat32_t poly = svmla_x (pg, sv_f32 (d->c0), r, sv_f32 (d->c1));
  poly = svmul_x (svptrue_b32 (), poly, r);

  /* Reconstruct y as 2^(n - 23) * (1 + r * poly(r)).  */
  svfloat32_t y = svmla_x (pg, scale, scale, poly);

  /* Scale the result by 2^23:
     2^n * (1 + r * poly(r))
     = 2^23 * 2^(n - 23) * (1 + r * poly(r)).  */
  return svscale_x (pg, y, scale_adjust);
}

/* Vector version of exp2f.
   The maximum observed error is 0.59 + 0.5 ULP.
   _ZGVsMxv_exp2f (-0x1.97fa44p-3)
    got 0x1.bdf77cp-1
   want 0x1.bdf77ap-1.  */
svfloat32_t SV_NAME_F1 (exp2) (svfloat32_t x, const svbool_t pg)
{
  const struct data *d = ptr_barrier (&data);
  svbool_t special = svacgt (pg, x, d->special_bound);
  if (unlikely (svptest_any (special, special)))
    return special_case (x, pg, d);
  return sv_exp2f_inline (x, pg, d);
}

TEST_SIG (SV, F, 1, exp2, -9.9, 9.9)
TEST_ULP (SV_NAME_F1 (exp2), 0.59)
/* Positive x.  */
TEST_INTERVAL (SV_NAME_F1 (exp2), 0, SpecialBound, 50000)
TEST_INTERVAL (SV_NAME_F1 (exp2), SpecialBound, InfBound, 50000)
TEST_INTERVAL (SV_NAME_F1 (exp2), InfBound, inf, 50000)
/* Negative x.  */
TEST_INTERVAL (SV_NAME_F1 (exp2), -0, ZeroBound, 50000)
TEST_INTERVAL (SV_NAME_F1 (exp2), ZeroBound, -inf, 50000)
/* Full range including NaNs.  */
TEST_INTERVAL (SV_NAME_F1 (exp2), 0, 0xffff0000, 50000)
CLOSE_SVE_ATTR
