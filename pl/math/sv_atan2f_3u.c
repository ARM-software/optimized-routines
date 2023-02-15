/*
 * Single-precision vector atan2f(x) function.
 *
 * Copyright (c) 2021-2023, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "sv_math.h"
#include "pl_sig.h"
#include "pl_test.h"

#if SV_SUPPORTED

#include "sv_atanf_common.h"

/* Useful constants.  */
#define PiOver2 sv_f32 (0x1.921fb6p+0f)
#define SignMask sv_u32 (0x80000000)

/* Special cases i.e. 0, infinity, nan (fall back to scalar calls).  */
static inline svfloat32_t
specialcase (svfloat32_t y, svfloat32_t x, svfloat32_t ret, const svbool_t cmp)
{
  return sv_call2_f32 (atan2f, y, x, ret, cmp);
}

/* Returns a predicate indicating true if the input is the bit representation of
   0, infinity or nan.  */
static inline svbool_t
zeroinfnan (svuint32_t i, const svbool_t pg)
{
  return svcmpge_u32 (pg, svsub_n_u32_x (pg, svlsl_n_u32_x (pg, i, 1), 1),
		      sv_u32 (2 * 0x7f800000lu - 1));
}

/* Fast implementation of SVE atan2f based on atan(x) ~ shift + z + z^3 * P(z^2)
   with reduction to [0,1] using z=1/x and shift = pi/2.
   Maximum observed error is 2.95 ULP:
   SV_NAME_F2 (atan2)(0x1.93836cp+6, 0x1.8cae1p+6) got 0x1.967f06p-1
					   want 0x1.967f00p-1.  */
svfloat32_t SV_NAME_F2 (atan2) (svfloat32_t y, svfloat32_t x, const svbool_t pg)
{
  svuint32_t ix = svreinterpret_u32_f32 (x);
  svuint32_t iy = svreinterpret_u32_f32 (y);

  svbool_t cmp_x = zeroinfnan (ix, pg);
  svbool_t cmp_y = zeroinfnan (iy, pg);
  svbool_t cmp_xy = svorr_b_z (pg, cmp_x, cmp_y);

  svuint32_t sign_x = svand_u32_x (pg, ix, SignMask);
  svuint32_t sign_y = svand_u32_x (pg, iy, SignMask);
  svuint32_t sign_xy = sveor_u32_x (pg, sign_x, sign_y);

  svfloat32_t ax = svabs_f32_x (pg, x);
  svfloat32_t ay = svabs_f32_x (pg, y);

  svbool_t pred_xlt0 = svcmplt_f32 (pg, x, sv_f32 (0.0));
  svbool_t pred_aygtax = svcmpgt_f32 (pg, ay, ax);

  /* Set up z for call to atan.  */
  svfloat32_t n = svsel_f32 (pred_aygtax, svneg_f32_x (pg, ax), ay);
  svfloat32_t d = svsel_f32 (pred_aygtax, ay, ax);
  svfloat32_t z = svdiv_f32_x (pg, n, d);

  /* Work out the correct shift.  */
  svfloat32_t shift = svsel_f32 (pred_xlt0, sv_f32 (-2.0), sv_f32 (0.0));
  shift = svsel_f32 (pred_aygtax, svadd_n_f32_x (pg, shift, 1.0), shift);
  shift = svmul_f32_x (pg, shift, PiOver2);

  svfloat32_t ret = __sv_atanf_common (pg, pg, z, z, shift);

  /* Account for the sign of x and y.  */
  ret = svreinterpret_f32_u32 (
    sveor_u32_x (pg, svreinterpret_u32_f32 (ret), sign_xy));

  if (unlikely (svptest_any (pg, cmp_xy)))
    {
      return specialcase (y, x, ret, cmp_xy);
    }

  return ret;
}

/* Arity of 2 means no mathbench entry emitted. See test/mathbench_funcs.h.  */
PL_SIG (SV, F, 2, atan2)
PL_TEST_ULP (SV_NAME_F2 (atan2), 2.45)
PL_TEST_INTERVAL (SV_NAME_F2 (atan2), -10.0, 10.0, 50000)
PL_TEST_INTERVAL (SV_NAME_F2 (atan2), -1.0, 1.0, 40000)
PL_TEST_INTERVAL (SV_NAME_F2 (atan2), 0.0, 1.0, 40000)
PL_TEST_INTERVAL (SV_NAME_F2 (atan2), 1.0, 100.0, 40000)
PL_TEST_INTERVAL (SV_NAME_F2 (atan2), 1e6, 1e32, 40000)
#endif
