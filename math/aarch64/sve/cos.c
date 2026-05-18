/*
 * Double-precision SVE cos(x) function.
 *
 * Copyright (c) 2019-2026, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "sv_math.h"
#include "test_sig.h"
#include "test_defs.h"

static const struct data
{
  double inv_pio2, pio2_1, pio2_2, pio2_3, shift, range_val;
} data = {
  /* Polynomial coefficients are hardwired in FTMAD instructions.  */
  .inv_pio2 = 0x1.45f306dc9c882p-1,
  .pio2_1 = 0x1.921fb54442d18p+0,
  .pio2_2 = 0x1.1a62633145c07p-54,
  .pio2_3 = -0x1.f1976b7ed8fbcp-110,
  /* Original shift used in AdvSIMD cos,
     plus a contribution to set the bit #0 of q
     as expected by trigonometric instructions.  */
  .shift = 0x1.8000000000001p52,
  .range_val = 0x1p23,
};

static svfloat64_t NOINLINE
special_case (svfloat64_t x, svfloat64_t y, svbool_t cmp)
{
  return sv_call_f64 (cos, x, y, cmp);
}

/* Vector version of cos.
   The maximum observed error is 1.53 + 0.5 ULP when |x| < 0x1p23.
   _ZGVsMxv_cos (0x1.166b1063318b8p+19)
    got 0x1.fff1e92b6c31ap-4
   want 0x1.fff1e92b6c318p-4.  */
svfloat64_t SV_NAME_D1 (cos) (svfloat64_t x, const svbool_t pg)
{
  const struct data *d = ptr_barrier (&data);
  svfloat64_t inv_pio2 = svld1rq (svptrue_b64 (), &d->inv_pio2);
  svfloat64_t pio2_23 = svld1rq (svptrue_b64 (), &d->pio2_2);

  /* n = rint(x/(pi/2)).  */
  svfloat64_t q = svmla_lane (sv_f64 (d->shift), x, inv_pio2, 0);
  svfloat64_t n = svsub_x (pg, q, sv_f64 (d->shift));

  /* r = x - n*(pi/2)  (range reduction into -pi/4 .. pi/4).  */
  svfloat64_t r = x;
  r = svmls_lane (r, n, inv_pio2, 1);
  r = svmls_lane (r, n, pio2_23, 0);
  r = svmls_lane (r, n, pio2_23, 1);

  /* sin(r) or cos(r) poly approx, selected by the quadrant bits in q.  */
  svfloat64_t r2 = svtsmul (r, svreinterpret_u64 (q));
  svfloat64_t y = sv_f64 (0.0);
  y = svtmad (y, r2, 7);
  y = svtmad (y, r2, 6);
  y = svtmad (y, r2, 5);
  y = svtmad (y, r2, 4);
  y = svtmad (y, r2, 3);
  y = svtmad (y, r2, 2);
  y = svtmad (y, r2, 1);
  y = svtmad (y, r2, 0);

  /* Final multiplicative factor: 1.0 or x depending on bit #0 of q.  */
  svfloat64_t f = svtssel (r, svreinterpret_u64 (q));

  svbool_t special = svacge (pg, x, d->range_val);
  if (unlikely (svptest_any (pg, special)))
    return special_case (x, svmul_x (svptrue_b64 (), f, y), special);

  /* Apply factor.  */
  return svmul_x (svptrue_b64 (), f, y);
}

TEST_SIG (SV, D, 1, cos, -3.1, 3.1)
TEST_ULP (SV_NAME_D1 (cos), 1.53)
TEST_INTERVAL (SV_NAME_D1 (cos), 0, 0xffff0000, 10000)
TEST_INTERVAL (SV_NAME_D1 (cos), 0x1p-4, 0x1p4, 500000)
CLOSE_SVE_ATTR
