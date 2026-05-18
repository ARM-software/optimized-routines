/*
 * Single-precision vector sincos function.
 *
 * Copyright (c) 2023-2026, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

/* Define _GNU_SOURCE in order to include sincosf declaration. If building
   pre-GLIBC 2.1, or on a non-GNU conforming system, this routine will need to
   be linked against the scalar sincosf from math/.  */
#define _GNU_SOURCE

#include "sv_math.h"
#include "sv_sincosf_common.h"
#include "test_defs.h"

#include <math.h>

/* sincos not available for all scalar libm implementations.  */
#ifndef __GLIBC__
static void
sincosf (float x, float *out_sin, float *out_cos)
{
  *out_sin = sinf (x);
  *out_cos = cosf (x);
}
#endif

static void NOINLINE
special_case (svfloat32_t x, svbool_t special, float *out_sin, float *out_cos)
{
  svbool_t p = svptrue_pat_b32 (SV_VL1);
  for (int i = 0; i < svcntw (); i++)
    {
      if (svptest_any (special, p))
	sincosf (svlastb (p, x), out_sin + i, out_cos + i);
      p = svpnext_b32 (svptrue_b32 (), p);
    }
}

/* Single-precision vector function allowing calculation of both sin and cos in
   one function call, using shared argument reduction and separate low-order
   polynomials.
   The maximum observed error is 1.44 + 0.5 ULP for sin when |x| < 0x1p20.
   _ZGVsMxvl4l4_sincosf_sin (0x1.4b0d9cp+13)
    got 0x1.fc28cep-3
   want 0x1.fc28d2p-3
   The maximum observed error is 1.56 + 0.5 ULP for cos when |x| < 0x1p20.
   _ZGVsMxvl4l4_sincosf_cos (0x1.dea2f2p+19)
    got 0x1.fffe7ap-6
   want 0x1.fffe76p-6.  */
void
_ZGVsMxvl4l4_sincosf (svfloat32_t x, float *out_sin, float *out_cos,
		      svbool_t pg)
{
  const struct trig_data *d = ptr_barrier (&trig_data);
  svbool_t special = svacge (pg, x, d->range_val);

  svfloat32x2_t result = sv_sincosf_inline (x, d);
  svst1_f32 (pg, out_sin, svget2 (result, 0));
  svst1_f32 (pg, out_cos, svget2 (result, 1));

  if (unlikely (svptest_any (pg, special)))
    special_case (x, special, out_sin, out_cos);
}

TEST_ULP (_ZGVsMxvl4l4_sincosf_sin, 1.45)
TEST_ULP (_ZGVsMxvl4l4_sincosf_cos, 1.57)
#define SV_SINCOSF_INTERVAL(lo, hi, n)                                        \
  TEST_SYM_INTERVAL (_ZGVsMxvl4l4_sincosf_sin, lo, hi, n)                     \
  TEST_SYM_INTERVAL (_ZGVsMxvl4l4_sincosf_cos, lo, hi, n)
SV_SINCOSF_INTERVAL (0, 0x1p-31, 50000)
SV_SINCOSF_INTERVAL (0x1p-31, 0x1p20, 500000)
SV_SINCOSF_INTERVAL (0x1p20, inf, 10000)
CLOSE_SVE_ATTR
