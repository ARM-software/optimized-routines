/*
 * Single-precision vector log(x + 1) function.
 *
 * Copyright (c) 2023-2026, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "sv_math.h"
#include "test_sig.h"
#include "test_defs.h"
#include "sv_log1pf_inline.h"

static svfloat32_t NOINLINE
special_case (svfloat32_t x, svfloat32_t y, svbool_t pg, svbool_t special,
	      const struct sv_log1pf_data *d)
{
  y = svsel_f32 (special, svreinterpret_f32 (sv_u32 (d->nan)), y);

  svbool_t ret_pinf = svcmpeq (pg, x, asfloat (d->inf));
  svbool_t ret_minf = svcmpeq (pg, x, -1.0f);

  y = svsel_f32 (ret_pinf, svreinterpret_f32 (sv_u32 (d->inf)), y);
  return svsel_f32 (ret_minf, sv_f32 (-d->inf), y);
}

/* Vector log1pf approximation using polynomial on reduced interval. Worst-case
   error is 1.27 ULP very close to 0.5.
   _ZGVsMxv_log1pf(0x1.fffffep-2) got 0x1.9f324p-2
				 want 0x1.9f323ep-2.  */
svfloat32_t SV_NAME_F1 (log1p) (svfloat32_t x, svbool_t pg)
{

  const struct sv_log1pf_data *d = ptr_barrier (&sv_log1pf_data);
  /* x < -1, Inf/Nan.  */
  svbool_t special = svcmpeq (pg, svreinterpret_u32 (x), d->inf);
  special = svorn_z (pg, special, svcmpge (pg, x, -1.0f));

  if (unlikely (svptest_any (pg, special)))
    return special_case (x, sv_log1pf_inline (x, pg), pg, special, d);

  return sv_log1pf_inline (x, pg);
}

TEST_SIG (SV, F, 1, log1p, -0.9, 10.0)
TEST_ULP (SV_NAME_F1 (log1p), 0.77)
TEST_SYM_INTERVAL (SV_NAME_F1 (log1p), 0, 0x1p-23, 5000)
TEST_SYM_INTERVAL (SV_NAME_F1 (log1p), 0x1p-23, 1, 5000)
TEST_INTERVAL (SV_NAME_F1 (log1p), 1, inf, 10000)
TEST_INTERVAL (SV_NAME_F1 (log1p), -1, -inf, 10)
CLOSE_SVE_ATTR
