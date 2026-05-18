/*
 * Single-precision SVE cos(x) function.
 *
 * Copyright (c) 2019-2026, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "sv_math.h"
#include "test_sig.h"
#include "test_defs.h"

static const struct data
{
  float neg_pio2_1, neg_pio2_2, neg_pio2_3, inv_pio2, shift, range_val;
} data = {
  /* Polynomial coefficients are hard-wired in FTMAD instructions.  */
  .neg_pio2_1 = -0x1.921fb6p+0f,
  .neg_pio2_2 = 0x1.777a5cp-25f,
  .neg_pio2_3 = 0x1.ee59dap-50f,
  .inv_pio2 = 0x1.45f306p-1f,
  /* Original shift used in AdvSIMD cosf,
     plus a contribution to set the bit #0 of q
     as expected by trigonometric instructions.  */
  .shift = 0x1.800002p+23f,
  .range_val = 0x1p20f,
};

static svfloat32_t NOINLINE
special_case (svfloat32_t x, svfloat32_t y, svbool_t oob)
{
  return sv_call_f32 (cosf, x, y, oob);
}

/* Vector version of cosf.
   The maximum observed error is 1.56 + 0.5 ULP if |x| < 0x1p20.
   _ZGVsMxv_cosf (0x1.dea2f2p+19)
    got 0x1.fffe7ap-6
   want 0x1.fffe76p-6.  */
svfloat32_t SV_NAME_F1 (cos) (svfloat32_t x, const svbool_t pg)
{
  const struct data *d = ptr_barrier (&data);
  svbool_t ptrue = svptrue_b32 ();

  /* Load some constants in quad-word chunks to minimise memory access.  */
  svfloat32_t negpio2_and_invpio2 = svld1rq (ptrue, &d->neg_pio2_1);

  /* n = rint(x/(pi/2)).  */
  svfloat32_t q = svmla_lane (sv_f32 (d->shift), x, negpio2_and_invpio2, 3);
  svfloat32_t n = svsub_x (ptrue, q, d->shift);

  /* r = x - n*(pi/2)  (range reduction into -pi/4 .. pi/4).  */
  svfloat32_t r = x;
  r = svmla_lane (r, n, negpio2_and_invpio2, 0);
  r = svmla_lane (r, n, negpio2_and_invpio2, 1);
  r = svmla_lane (r, n, negpio2_and_invpio2, 2);

  /* Final multiplicative factor: 1.0 or x depending on bit #0 of q.  */
  svuint32_t q_u = svreinterpret_u32 (q);
  svfloat32_t f = svtssel (r, q_u);

  /* cos(r) poly approx.  */
  svfloat32_t r2 = svtsmul (r, q_u);
  svfloat32_t y = sv_f32 (0.0f);
  y = svtmad (y, r2, 4);
  y = svtmad (y, r2, 3);
  y = svtmad (y, r2, 2);
  y = svtmad (y, r2, 1);
  y = svtmad (y, r2, 0);

  svbool_t cmp = svacge (pg, x, sv_f32 (d->range_val));
  if (unlikely (svptest_any (pg, cmp)))
    return special_case (x, svmul_x (ptrue, f, y), cmp);
  /* Apply factor.  */
  return svmul_x (ptrue, f, y);
}

TEST_SIG (SV, F, 1, cos, -3.1, 3.1)
TEST_ULP (SV_NAME_F1 (cos), 1.57)
TEST_INTERVAL (SV_NAME_F1 (cos), 0, 0xffff0000, 10000)
TEST_INTERVAL (SV_NAME_F1 (cos), 0x1p-4, 0x1p4, 500000)
CLOSE_SVE_ATTR
