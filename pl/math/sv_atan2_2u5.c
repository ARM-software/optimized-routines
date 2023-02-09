/*
 * Double-precision vector atan2(x) function.
 *
 * Copyright (c) 2021-2023, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "sv_math.h"
#include "pl_sig.h"
#include "pl_test.h"

#if SV_SUPPORTED

#include "sv_atan_common.h"

/* Useful constants.  */
#define PiOver2 sv_f64 (0x1.921fb54442d18p+0)
#define SignMask sv_u64 (0x8000000000000000)

/* Special cases i.e. 0, infinity, nan (fall back to scalar calls).  */
__attribute__ ((noinline)) static svfloat64_t
specialcase (svfloat64_t y, svfloat64_t x, svfloat64_t ret, const svbool_t cmp)
{
  return sv_call2_f64 (atan2, y, x, ret, cmp);
}

/* Returns a predicate indicating true if the input is the bit representation of
   0, infinity or nan.  */
static inline svbool_t
zeroinfnan (svuint64_t i, const svbool_t pg)
{
  return svcmpge_u64 (pg, svsub_n_u64_x (pg, svlsl_n_u64_x (pg, i, 1), 1),
		      sv_u64 (2 * asuint64 (INFINITY) - 1));
}

/* Fast implementation of SVE atan2. Errors are greatest when y and
   x are reasonably close together. The greatest observed error is 2.28 ULP:
   sv_atan2(-0x1.5915b1498e82fp+732, 0x1.54d11ef838826p+732)
   got -0x1.954f42f1fa841p-1 want -0x1.954f42f1fa843p-1.  */
svfloat64_t SV_NAME_D2 (atan2) (svfloat64_t y, svfloat64_t x, const svbool_t pg)
{
  svuint64_t ix = sv_as_u64_f64 (x);
  svuint64_t iy = sv_as_u64_f64 (y);

  svbool_t cmp_x = zeroinfnan (ix, pg);
  svbool_t cmp_y = zeroinfnan (iy, pg);
  svbool_t cmp_xy = svorr_b_z (pg, cmp_x, cmp_y);

  svuint64_t sign_x = svand_u64_x (pg, ix, SignMask);
  svuint64_t sign_y = svand_u64_x (pg, iy, SignMask);
  svuint64_t sign_xy = sveor_u64_x (pg, sign_x, sign_y);

  svfloat64_t ax = svabs_f64_x (pg, x);
  svfloat64_t ay = svabs_f64_x (pg, y);

  svbool_t pred_xlt0 = svcmplt_f64 (pg, x, sv_f64 (0.0));
  svbool_t pred_aygtax = svcmpgt_f64 (pg, ay, ax);

  /* Set up z for call to atan.  */
  svfloat64_t n = svsel_f64 (pred_aygtax, svneg_f64_x (pg, ax), ay);
  svfloat64_t d = svsel_f64 (pred_aygtax, ay, ax);
  svfloat64_t z = svdiv_f64_x (pg, n, d);

  /* Work out the correct shift.  */
  svfloat64_t shift = svsel_f64 (pred_xlt0, sv_f64 (-2.0), sv_f64 (0.0));
  shift = svsel_f64 (pred_aygtax, svadd_n_f64_x (pg, shift, 1.0), shift);
  shift = svmul_f64_x (pg, shift, PiOver2);

  svfloat64_t ret = __sv_atan_common (pg, pg, z, z, shift);

  /* Account for the sign of x and y.  */
  ret = sv_as_f64_u64 (sveor_u64_x (pg, sv_as_u64_f64 (ret), sign_xy));

  if (unlikely (svptest_any (pg, cmp_xy)))
    {
      return specialcase (y, x, ret, cmp_xy);
    }

  return ret;
}

/* Arity of 2 means no mathbench entry emitted. See test/mathbench_funcs.h.  */
PL_SIG (SV, D, 2, atan2)
PL_TEST_ULP (SV_NAME_D2 (atan2), 1.78)
PL_TEST_INTERVAL (SV_NAME_D2 (atan2), -10.0, 10.0, 50000)
PL_TEST_INTERVAL (SV_NAME_D2 (atan2), -1.0, 1.0, 40000)
PL_TEST_INTERVAL (SV_NAME_D2 (atan2), 0.0, 1.0, 40000)
PL_TEST_INTERVAL (SV_NAME_D2 (atan2), 1.0, 100.0, 40000)
PL_TEST_INTERVAL (SV_NAME_D2 (atan2), 1e6, 1e32, 40000)
#endif
