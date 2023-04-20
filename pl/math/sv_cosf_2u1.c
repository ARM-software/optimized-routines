/*
 * Single-precision SVE cos(x) function.
 *
 * Copyright (c) 2019-2023, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "sv_math.h"
#include "pl_sig.h"
#include "pl_test.h"

#if SV_SUPPORTED

struct sv_cosf_data
{
  float neg_pio2_1, neg_pio2_2, neg_pio2_3, inv_pio2, shift;
};

static struct sv_cosf_data data
  = {.neg_pio2_1 = -0x1.921fb6p+0f,
     .neg_pio2_2 = 0x1.777a5cp-25f,
     .neg_pio2_3 = 0x1.ee59dap-50f,
     .inv_pio2 = 0x1.45f306p-1f,
     /* Original shift used in Neon cosf,
	plus a contribution to set the bit #0 of q
	as expected by trigonometric instructions.  */
     .shift = 0x1.800002p+23f};

#define RangeVal 0x49800000 /* asuint32(0x1p20f).  */

static svfloat32_t NOINLINE
special_case (svfloat32_t x, svfloat32_t y, svbool_t out_of_bounds)
{
  return sv_call_f32 (cosf, x, y, out_of_bounds);
}

/* A fast SVE implementation of cosf based on trigonometric
   instructions (FTMAD, FTSSEL, FTSMUL).
   Maximum measured error: 2.06 ULPs.
   SV_NAME_F1 (cos)(0x1.dea2f2p+19) got 0x1.fffe7ap-6
				   want 0x1.fffe76p-6.  */
svfloat32_t SV_NAME_F1 (cos) (svfloat32_t x, const svbool_t pg)
{
  svfloat32_t r = svabs_f32_x (pg, x);
  svbool_t out_of_bounds
    = svcmpge_n_u32 (pg, svreinterpret_u32_f32 (r), RangeVal);

  /* Load some constants in quad-word chunks to minimise memory access.  */
  svfloat32_t negpio2_and_invpio2 = svld1rq_f32 (pg, &data.neg_pio2_1);

  /* n = rint(|x|/(pi/2)).  */
  svfloat32_t q
    = svmla_lane_f32 (sv_f32 (data.shift), r, negpio2_and_invpio2, 3);
  svfloat32_t n = svsub_n_f32_x (pg, q, data.shift);

  /* r = |x| - n*(pi/2)  (range reduction into -pi/4 .. pi/4).  */
  r = svmla_lane_f32 (r, n, negpio2_and_invpio2, 0);
  r = svmla_lane_f32 (r, n, negpio2_and_invpio2, 1);
  r = svmla_lane_f32 (r, n, negpio2_and_invpio2, 2);

  /* Final multiplicative factor: 1.0 or x depending on bit #0 of q.  */
  svfloat32_t f = svtssel_f32 (r, svreinterpret_u32_f32 (q));

  /* cos(r) poly approx.  */
  svfloat32_t r2 = svtsmul_f32 (r, svreinterpret_u32_f32 (q));
  svfloat32_t y = sv_f32 (0.0f);
  y = svtmad_f32 (y, r2, 4);
  y = svtmad_f32 (y, r2, 3);
  y = svtmad_f32 (y, r2, 2);
  y = svtmad_f32 (y, r2, 1);
  y = svtmad_f32 (y, r2, 0);

  /* Apply factor.  */
  y = svmul_f32_x (pg, f, y);

  if (unlikely (svptest_any (pg, out_of_bounds)))
    return special_case (x, y, out_of_bounds);
  return y;
}

PL_SIG (SV, F, 1, cos, -3.1, 3.1)
PL_TEST_ULP (SV_NAME_F1 (cos), 1.57)
PL_TEST_INTERVAL (SV_NAME_F1 (cos), 0, 0xffff0000, 10000)
PL_TEST_INTERVAL (SV_NAME_F1 (cos), 0x1p-4, 0x1p4, 500000)
#endif
