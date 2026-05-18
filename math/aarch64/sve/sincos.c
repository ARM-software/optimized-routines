/*
 * Double-precision vector sincos function.
 *
 * Copyright (c) 2023-2026, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

/* Define _GNU_SOURCE in order to include sincos declaration. If building
   pre-GLIBC 2.1, or on a non-GNU conforming system, this routine will need to
   be linked against the scalar sincosf from math/.  */
#define _GNU_SOURCE

#include "sv_math.h"
#include "sv_sincos_common.h"
#include "test_defs.h"

#include <math.h>

/* sincos not available for all scalar libm implementations.  */
#ifndef __GLIBC__
static void
sincos (double x, double *out_sin, double *out_cos)
{
  *out_sin = sin (x);
  *out_cos = cos (x);
}
#endif

static void NOINLINE
special_case (svfloat64_t x, svbool_t special, double *out_sin,
	      double *out_cos)
{
  svbool_t p = svptrue_pat_b64 (SV_VL1);
  for (int i = 0; i < svcntd (); i++)
    {
      if (svptest_any (special, p))
	sincos (svlastb (p, x), out_sin + i, out_cos + i);
      p = svpnext_b64 (svptrue_b64 (), p);
    }
}

/* Double-precision vector function allowing calculation of both sin and cos in
   one function call, using shared argument reduction and addition angle trig
   identities.
   Worst-case error for sin is 2.67 + 0.5 ULP when |x| < 0x1p23:
   _ZGVsMxvl8l8_sincos_sin (0x1.022ae05e6dae2p+12)
    got 0x1.f7f7190cb05e9p-2
   want 0x1.f7f7190cb05e6p-2
   Worst-case error for cos is 2.68 + 0.5 ULP when |x| < 0x1p23:
   _ZGVsMxvl8l8_sincos_cos (0x1.d187aa5fac8f7p+3)
    got -0x1.98c7cd085e031p-2
   want -0x1.98c7cd085e02ep-2.  */
void
_ZGVsMxvl8l8_sincos (svfloat64_t x, double *out_sin, double *out_cos,
		     svbool_t pg)
{
  const struct sv_sincos_data *d = ptr_barrier (&sv_sincos_data);

  svfloat64x2_t sc = sv_sincos_inline (x, d);

  svst1 (pg, out_sin, svget2 (sc, 0));
  svst1 (pg, out_cos, svget2 (sc, 1));

  svbool_t special = svacgt (pg, x, d->range_val);
  if (unlikely (svptest_any (pg, special)))
    special_case (x, special, out_sin, out_cos);
}

TEST_ULP (_ZGVsMxvl8l8_sincos_sin, 2.68)
TEST_ULP (_ZGVsMxvl8l8_sincos_cos, 2.69)
#define SV_SINCOS_INTERVAL(lo, hi, n)                                         \
  TEST_SYM_INTERVAL (_ZGVsMxvl8l8_sincos_sin, lo, hi, n)                      \
  TEST_SYM_INTERVAL (_ZGVsMxvl8l8_sincos_cos, lo, hi, n)
SV_SINCOS_INTERVAL (0, 0x1p-63, 50000)
SV_SINCOS_INTERVAL (0x1p-63, 0x1p23, 500000)
SV_SINCOS_INTERVAL (0x1p23, inf, 10000)
CLOSE_SVE_ATTR
