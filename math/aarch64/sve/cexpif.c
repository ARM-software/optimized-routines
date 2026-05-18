/*
 * Single-precision vector cexpi function.
 *
 * Copyright (c) 2023-2026, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "sv_math.h"
#include "sv_sincosf_common.h"
#include "test_defs.h"

/* Wrapper to prevent inlining.  */
static svfloat32x2_t NOINLINE
special_case (svfloat32_t x, svbool_t special, const struct trig_data *d)
{
  return sv_sincos_fallback (x, special, d);
}

/* Single-precision vector function allowing calculation of both sin and cos in
   one function call, using shared argument reduction and separate low-order
   polynomials.
   The maximum observed error is 1.44 + 0.5 ULP for sin when |x| < 0x1p20.
   _ZGVsMxv_cexpif_sin (0x1.4b0d9cp+13)
    got 0x1.fc28cep-3
   want 0x1.fc28d2p-3
   The maximum observed error is 1.56 + 0.5 ULP for cos when |x| < 0x1p20.
   _ZGVsMxv_cexpif_cos (0x1.dea2f2p+19)
    got 0x1.fffe7ap-6
   want 0x1.fffe76p-6
   The special domain has a higher maximum error than the fast path:
   The maximum observed error is 2.69 + 0.5 ULP for sin when |x| >= 0x1p20.
   _ZGVsMxv_cexpif_sin (0x1.be07aap+77)
    got 0x1.ffe05ep-5
   want 0x1.ffe058p-5
   The maximum observed error is 2.65 + 0.5 ULP for cos when |x| >= 0x1p20.
   _ZGVsMxv_cexpif_cos (0x1.ff3afcp+53)
    got -0x1.ffe74p-3
   want -0x1.ffe73ap-3.  */
svfloat32x2_t
_ZGVsMxv_cexpif (svfloat32_t x, svbool_t pg)
{
  const struct trig_data *d = ptr_barrier (&trig_data);
  svbool_t special = svacge (pg, x, d->range_val);

  if (unlikely (svptest_any (pg, special)))
    return special_case (x, special, d);
  return sv_sincosf_inline (x, d);
}

TEST_ULP (_ZGVsMxv_cexpif_sin, 2.70)
TEST_ULP (_ZGVsMxv_cexpif_cos, 2.65)
#define SV_CEXPIF_INTERVAL(lo, hi, n)                                         \
  TEST_INTERVAL (_ZGVsMxv_cexpif_sin, lo, hi, n)                              \
  TEST_INTERVAL (_ZGVsMxv_cexpif_cos, lo, hi, n)
SV_CEXPIF_INTERVAL (0, 0x1p20, 500000)
SV_CEXPIF_INTERVAL (-0, -0x1p20, 500000)
SV_CEXPIF_INTERVAL (0x1p20, inf, 10000)
SV_CEXPIF_INTERVAL (-0x1p20, -inf, 10000)
CLOSE_SVE_ATTR
