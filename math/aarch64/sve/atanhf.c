/*
 * Single-precision vector atanh(x) function.
 *
 * Copyright (c) 2023-2026, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "sv_math.h"
#include "test_sig.h"
#include "test_defs.h"

#include "sv_log1pf_inline.h"

static svfloat32_t inline special_case (svfloat32_t ax, svfloat32_t y,
					svbool_t special, svfloat32_t halfsign,
					const struct sv_log1pf_data *d)
{
  svfloat32_t res = svsel (special, svreinterpret_f32 (sv_u32 (d->nan)), y);
  res = svsel (svcmpeq (special, ax, sv_f32 (1.0)),
	       svreinterpret_f32 (sv_u32 (d->inf)), res);
  return svmul_x (svptrue_b32 (), res, halfsign);
}

/* Approximation for vector single-precision atanh(x) using modified log1p.
   The maximum error is 1.99 ULP:
   _ZGVsMxv_atanhf(0x1.f1583p-5) got 0x1.f1f4fap-5
				want 0x1.f1f4f6p-5.  */
svfloat32_t SV_NAME_F1 (atanh) (svfloat32_t x, const svbool_t pg)
{
  const struct sv_log1pf_data *d = ptr_barrier (&sv_log1pf_data);

  svfloat32_t ax = svabs_x (pg, x);
  svuint32_t iax = svreinterpret_u32 (ax);
  svuint32_t sign = sveor_x (svptrue_b32 (), svreinterpret_u32 (x), iax);
  svfloat32_t halfsign
      = svreinterpret_f32 (svorr_x (svptrue_b32 (), sign, 0x3f000000));
  svbool_t special = svcmpge (pg, iax, svreinterpret_u32 (sv_f32 (1)));

  /* Computation is performed based on the following sequence of equality:
     (1+x)/(1-x) = 1 + 2x/(1-x).  */
  svfloat32_t y = svadd_x (svptrue_b32 (), ax, ax);
  y = svdiv_x (pg, y, svsubr_x (pg, ax, 1.0f));
  /* ln((1+x)/(1-x)) = ln(1+2x/(1-x)) = ln(1 + y).  */
  y = sv_log1pf_inline (y, pg);

  if (unlikely (svptest_any (pg, special)))
    return special_case (ax, y, special, halfsign, d);

  return svmul_x (svptrue_b32 (), halfsign, y);
}

TEST_SIG (SV, F, 1, atanh, -1.0, 1.0)
TEST_ULP (SV_NAME_F1 (atanh), 1.50)
TEST_SYM_INTERVAL (SV_NAME_F1 (atanh), 0, 0x1p-12, 1000)
TEST_SYM_INTERVAL (SV_NAME_F1 (atanh), 0x1p-12, 1, 20000)
TEST_SYM_INTERVAL (SV_NAME_F1 (atanh), 1, inf, 1000)
/* atanh is asymptotic at 1, which is the default control value - have to set
 -c 0 specially to ensure fp exceptions are triggered correctly (choice of
 control lane is irrelevant if fp exceptions are disabled).  */
TEST_CONTROL_VALUE (SV_NAME_F1 (atanh), 0)
CLOSE_SVE_ATTR
