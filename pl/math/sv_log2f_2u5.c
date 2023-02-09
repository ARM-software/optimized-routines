/*
 * Single-precision vector/SVE log2 function.
 *
 * Copyright (c) 2022-2023, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "sv_math.h"
#include "pl_sig.h"
#include "pl_test.h"
#include "sv_pairwise_hornerf.h"

#if SV_SUPPORTED

#define P(i) __v_log2f_data.poly[i]

#define Ln2 (0x1.62e43p-1f) /* 0x3f317218.  */
#define Min (0x00800000)
#define Max (0x7f800000)
#define Mask (0x007fffff)
#define Off (0x3f2aaaab) /* 0.666667.  */

static NOINLINE svfloat32_t
specialcase (svfloat32_t x, svfloat32_t y, svbool_t cmp)
{
  return sv_call_f32 (log2f, x, y, cmp);
}

/* Optimised implementation of SVE log2f, using the same algorithm
   and polynomial as Neon log2f. Maximum error is 2.48 ULPs:
   SV_NAME_F1 (log2)(0x1.558174p+0) got 0x1.a9be84p-2
			    want 0x1.a9be8p-2.  */
svfloat32_t SV_NAME_F1 (log2) (svfloat32_t x, const svbool_t pg)
{
  svuint32_t u = sv_as_u32_f32 (x);
  svbool_t special
    = svcmpge_u32 (pg, svsub_n_u32_x (pg, u, Min), sv_u32 (Max - Min));

  /* x = 2^n * (1+r), where 2/3 < 1+r < 4/3.  */
  u = svsub_n_u32_x (pg, u, Off);
  svfloat32_t n = svcvt_f32_s32_x (pg, svasr_n_s32_x (pg, sv_as_s32_u32 (u),
						      23)); /* Sign-extend.  */
  u = svand_n_u32_x (pg, u, Mask);
  u = svadd_n_u32_x (pg, u, Off);
  svfloat32_t r = svsub_n_f32_x (pg, sv_as_f32_u32 (u), 1.0f);

  /* y = log2(1+r) + n.  */
  svfloat32_t r2 = svmul_f32_x (pg, r, r);

  /* Evaluate polynomial using pairwise Horner scheme.  */
  svfloat32_t y = PAIRWISE_HORNER_8 (pg, r, r2, P);
  y = sv_fma_f32_x (pg, y, r, n);

  if (unlikely (svptest_any (pg, special)))
    return specialcase (x, y, special);
  return y;
}

PL_SIG (SV, F, 1, log2, 0.01, 11.1)
PL_TEST_ULP (SV_NAME_F1 (log2), 1.99)
PL_TEST_EXPECT_FENV_ALWAYS (SV_NAME_F1 (log2))
PL_TEST_INTERVAL (SV_NAME_F1 (log2), -0.0, -0x1p126, 4000)
PL_TEST_INTERVAL (SV_NAME_F1 (log2), 0.0, 0x1p-126, 4000)
PL_TEST_INTERVAL (SV_NAME_F1 (log2), 0x1p-126, 0x1p-23, 50000)
PL_TEST_INTERVAL (SV_NAME_F1 (log2), 0x1p-23, 1.0, 50000)
PL_TEST_INTERVAL (SV_NAME_F1 (log2), 1.0, 100, 50000)
PL_TEST_INTERVAL (SV_NAME_F1 (log2), 100, inf, 50000)

#endif // SV_SUPPORTED
