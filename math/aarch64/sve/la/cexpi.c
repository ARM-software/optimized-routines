/*
 * Double-precision vector cexpi function.
 *
 * Copyright (c) 2023-2026, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "sv_math.h"
#include "sv_sincos_common.h"
#include "test_defs.h"

static svfloat64x2_t NOINLINE
special_case (svfloat64_t x, svbool_t special, const struct sv_sincos_data *d)
{
  svfloat64x2_t small = sv_sincos_inline (x, d);
  svfloat64x2_t large = sv_sincos_fallback (x);

  /* Inf cases are handled correctly by the fast path, and incorrectly
  by the slow path. However, it's less costly to the fast path to
  handle them separately. So we do want to branch here for inf cases,
  but then use the fast path value anyway.  */
  special = svaclt (special, x, sv_f64 (INFINITY));
  svfloat64_t sin = svsel (special, svget2 (large, 0), svget2 (small, 0));
  svfloat64_t cos = svsel (special, svget2 (large, 1), svget2 (small, 1));

  return svcreate2 (sin, cos);
}

/* Double-precision vector function allowing calculation of both sin and cos in
   one function call, using shared argument reduction and addition angle trig
   identities.
   Worst-case error for sin is 2.15 + 0.5 ULP when |x| >= 0x1p23:
   _ZGVsMxv_cexpi_sin (0x1.3d4ded894041ep+784)
    got -0x1.fffa6b28930b5p-7
   want -0x1.fffa6b28930b2p-7
   Worst-case error for cos is 2.44 + 0.5 ULP when |x| >= 0x1p23:
   _ZGVsMxv_cexpi_cos (0x1.aac6f8bffec82p+206)
    got -0x1.98ecd0b3020bfp-7
   want -0x1.98ecd0b3020bcp-7.

   Worst-case error for sin is 2.67 + 0.5 ULP when |x| < 0x1p23:
   _ZGVsMxv_cexpi_sin (0x1.022ae05e6dae2p+12)
    got 0x1.f7f7190cb05e9p-2
   want 0x1.f7f7190cb05e6p-2
   Worst-case error for cos is 2.68 + 0.5 ULP when |x| < 0x1p23:
   _ZGVsMxv_cexpi_cos (0x1.d187aa5fac8f7p+3)
    got -0x1.98c7cd085e031p-2
   want -0x1.98c7cd085e02ep-2.  */
svfloat64x2_t
_ZGVsMxv_cexpi (svfloat64_t x, svbool_t pg)
{
  const struct sv_sincos_data *d = ptr_barrier (&sv_sincos_data);
  svbool_t special = svacgt (pg, x, d->range_val);
  if (unlikely (svptest_any (special, special)))
    return special_case (x, special, d);
  return sv_sincos_inline (x, d);
}

TEST_ULP (_ZGVsMxv_cexpi_sin, 2.68)
TEST_ULP (_ZGVsMxv_cexpi_cos, 2.69)
#define SV_CEXPI_INTERVAL(lo, hi, n)                                          \
  TEST_INTERVAL (_ZGVsMxv_cexpi_sin, lo, hi, n)                               \
  TEST_INTERVAL (_ZGVsMxv_cexpi_cos, lo, hi, n)
SV_CEXPI_INTERVAL (0, 0x1p23, 500000)
SV_CEXPI_INTERVAL (-0, -0x1p23, 500000)
SV_CEXPI_INTERVAL (0x1p23, inf, 10000)
SV_CEXPI_INTERVAL (-0x1p23, -inf, 10000)
CLOSE_SVE_ATTR
