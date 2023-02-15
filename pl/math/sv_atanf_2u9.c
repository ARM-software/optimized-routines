/*
 * Single-precision vector atan(x) function.
 *
 * Copyright (c) 2021-2023, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "sv_math.h"
#include "pl_sig.h"
#include "pl_test.h"

#if SV_SUPPORTED

#include "sv_atanf_common.h"

#define PiOver2 sv_f32 (0x1.921fb6p+0f)
#define AbsMask (0x7fffffff)

/* Fast implementation of SVE atanf based on
   atan(x) ~ shift + z + z^3 * P(z^2) with reduction to [0,1] using
   z=-1/x and shift = pi/2.
   Largest observed error is 2.9 ULP, close to +/-1.0:
   SV_NAME_F1 (atan)(0x1.0468f6p+0) got -0x1.967f06p-1
			    want -0x1.967fp-1.  */
svfloat32_t SV_NAME_F1 (atan) (svfloat32_t x, const svbool_t pg)
{
  /* No need to trigger special case. Small cases, infs and nans
     are supported by our approximation technique.  */
  svuint32_t ix = svreinterpret_u32_f32 (x);
  svuint32_t sign = svand_n_u32_x (pg, ix, ~AbsMask);

  /* Argument reduction:
     y := arctan(x) for x < 1
     y := pi/2 + arctan(-1/x) for x > 1
     Hence, use z=-1/a if x>=1, otherwise z=a.  */
  svbool_t red = svacgt_n_f32 (pg, x, 1.0f);
  /* Avoid dependency in abs(x) in division (and comparison).  */
  svfloat32_t z = svsel_f32 (red, svdiv_f32_x (pg, sv_f32 (-1.0f), x), x);
  /* Use absolute value only when needed (odd powers of z).  */
  svfloat32_t az = svabs_f32_x (pg, z);
  az = svneg_f32_m (az, red, az);

  svfloat32_t y = __sv_atanf_common (pg, red, z, az, PiOver2);

  /* y = atan(x) if x>0, -atan(-x) otherwise.  */
  return svreinterpret_f32_u32 (
    sveor_u32_x (pg, svreinterpret_u32_f32 (y), sign));
}

PL_SIG (SV, F, 1, atan, -3.1, 3.1)
PL_TEST_ULP (SV_NAME_F1 (atan), 2.9)
PL_TEST_INTERVAL (SV_NAME_F1 (atan), -10.0, 10.0, 50000)
PL_TEST_INTERVAL (SV_NAME_F1 (atan), -1.0, 1.0, 40000)
PL_TEST_INTERVAL (SV_NAME_F1 (atan), 0.0, 1.0, 40000)
PL_TEST_INTERVAL (SV_NAME_F1 (atan), 1.0, 100.0, 40000)
PL_TEST_INTERVAL (SV_NAME_F1 (atan), 1e6, 1e32, 40000)
#endif
