/*
 * Double-precision SVE cos(x) function.
 *
 * Copyright (c) 2019-2023, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "sv_math.h"
#include "pl_sig.h"
#include "pl_test.h"

#if SV_SUPPORTED

struct sv_cos_data
{
  double inv_pio2, pio2_1, pio2_2, pio2_3, shift;
};

static struct sv_cos_data data
  = {.inv_pio2 = 0x1.45f306dc9c882p-1,
     .pio2_1 = 0x1.921fb50000000p+0,
     .pio2_2 = 0x1.110b460000000p-26,
     .pio2_3 = 0x1.1a62633145c07p-54,
     /* Original shift used in Neon cos,
	plus a contribution to set the bit #0 of q
	as expected by trigonometric instructions.  */
     .shift = 0x1.8000000000001p52};

#define RangeVal 0x4160000000000000 /* asuint64 (0x1p23).  */

static svfloat64_t NOINLINE
special_case (svfloat64_t x, svfloat64_t y, svbool_t out_of_bounds)
{
  return sv_call_f64 (cos, x, y, out_of_bounds);
}

/* A fast SVE implementation of cos based on trigonometric
   instructions (FTMAD, FTSSEL, FTSMUL).
   Maximum measured error: 2.108 ULPs.
   SV_NAME_D1 (cos)(0x1.9b0ba158c98f3p+7) got -0x1.fddd4c65c7f07p-3
					 want -0x1.fddd4c65c7f05p-3.  */
svfloat64_t SV_NAME_D1 (cos) (svfloat64_t x, const svbool_t pg)
{
  svfloat64_t r = svabs_f64_x (pg, x);
  svbool_t out_of_bounds
    = svcmpge_n_u64 (pg, svreinterpret_u64_f64 (r), RangeVal);

  /* Load some constants in quad-word chunks to minimise memory access.  */
  svfloat64_t invpio2_and_pio2_1 = svld1rq_f64 (pg, &data.inv_pio2);
  svfloat64_t pio2_23 = svld1rq_f64 (pg, &data.pio2_2);

  /* n = rint(|x|/(pi/2)).  */
  svfloat64_t q
    = svmla_lane_f64 (sv_f64 (data.shift), r, invpio2_and_pio2_1, 0);
  svfloat64_t n = svsub_n_f64_x (pg, q, data.shift);

  /* r = |x| - n*(pi/2)  (range reduction into -pi/4 .. pi/4).  */
  r = svmls_lane_f64 (r, n, invpio2_and_pio2_1, 1);
  r = svmls_lane_f64 (r, n, pio2_23, 0);
  r = svmls_lane_f64 (r, n, pio2_23, 1);

  /* cos(r) poly approx.  */
  svfloat64_t r2 = svtsmul_f64 (r, svreinterpret_u64_f64 (q));
  svfloat64_t y = sv_f64 (0.0);
  y = svtmad_f64 (y, r2, 7);
  y = svtmad_f64 (y, r2, 6);
  y = svtmad_f64 (y, r2, 5);
  y = svtmad_f64 (y, r2, 4);
  y = svtmad_f64 (y, r2, 3);
  y = svtmad_f64 (y, r2, 2);
  y = svtmad_f64 (y, r2, 1);
  y = svtmad_f64 (y, r2, 0);

  /* Final multiplicative factor: 1.0 or x depending on bit #0 of q.  */
  svfloat64_t f = svtssel_f64 (r, svreinterpret_u64_f64 (q));
  /* Apply factor.  */
  y = svmul_f64_x (pg, f, y);

  if (unlikely (svptest_any (pg, out_of_bounds)))
    return special_case (x, y, out_of_bounds);
  return y;
}

PL_SIG (SV, D, 1, cos, -3.1, 3.1)
PL_TEST_ULP (SV_NAME_D1 (cos), 1.61)
PL_TEST_INTERVAL (SV_NAME_D1 (cos), 0, 0xffff0000, 10000)
PL_TEST_INTERVAL (SV_NAME_D1 (cos), 0x1p-4, 0x1p4, 500000)
#endif
