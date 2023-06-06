/*
 * Single-precision SVE 2^x function.
 *
 * Copyright (c) 2023, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "sv_math.h"
#include "pl_sig.h"
#include "pl_test.h"

#define C(i) __sv_exp2f_poly[i]

#define Shift sv_f32 (0x1.903f8p17f) /* 1.5*2^17 + 127.  */
#define Thres                                                                  \
  (0x1.5d5e2ap+6f) /* Roughly 87.3. For x < -Thres, the result is subnormal    \
		      and not handled correctly by FEXPA.  */

static NOINLINE svfloat32_t
special_case (svfloat32_t x, svfloat32_t y, svbool_t special)
{
  return sv_call_f32 (exp2f, x, y, special);
}
/* Single-precision SVE exp2f routine. Implements the same algorithm
   as AdvSIMD exp2f.
   Worst case error is 1.04 ULPs.
   SV_NAME_F1 (exp2)(0x1.943b9p-1) got 0x1.ba7eb2p+0
				  want 0x1.ba7ebp+0.  */
svfloat32_t SV_NAME_F1 (exp2) (svfloat32_t x, const svbool_t pg)
{
  /* exp2(x) = 2^n (1 + poly(r)), with 1 + poly(r) in [1/sqrt(2),sqrt(2)]
    x = n + r, with r in [-1/2, 1/2].  */
  svfloat32_t z = svadd_f32_x (pg, x, Shift);
  svfloat32_t n = svsub_f32_x (pg, z, Shift);
  svfloat32_t r = svsub_f32_x (pg, x, n);

  svbool_t is_special_case = svacgt_n_f32 (pg, x, Thres);
  svfloat32_t scale = svexpa_f32 (svreinterpret_u32_f32 (z));

  /* Polynomial evaluation: poly(r) ~ exp(r)-1.  */
  svfloat32_t r2 = svmul_f32_x (pg, r, r);
  svfloat32_t p = svmla_n_f32_x (pg, sv_f32 (C (1)), r, C (0));
  svfloat32_t q = svmla_n_f32_x (pg, sv_f32 (C (3)), r, C (2));
  q = svmla_f32_x (pg, q, r2, p);
  p = svmul_n_f32_x (pg, r, C (4));
  svfloat32_t poly = svmla_f32_x (pg, p, r2, q);

  if (unlikely (svptest_any (pg, is_special_case)))
      return special_case (x, svmla_f32_x (pg, scale, scale, poly),
			   is_special_case);

  return svmla_f32_x (pg, scale, scale, poly);
}

PL_SIG (SV, F, 1, exp2, -9.9, 9.9)
PL_TEST_ULP (SV_NAME_F1 (exp2), 0.55)
PL_TEST_INTERVAL (SV_NAME_F1 (exp2), 0, Thres, 40000)
PL_TEST_INTERVAL (SV_NAME_F1 (exp2), Thres, 1, 50000)
PL_TEST_INTERVAL (SV_NAME_F1 (exp2), 1, Thres, 50000)
PL_TEST_INTERVAL (SV_NAME_F1 (exp2), Thres, inf, 50000)
PL_TEST_INTERVAL (SV_NAME_F1 (exp2), -0, -0x1p-23, 40000)
PL_TEST_INTERVAL (SV_NAME_F1 (exp2), -0x1p-23, -1, 50000)
PL_TEST_INTERVAL (SV_NAME_F1 (exp2), -1, -0x1p23, 50000)
PL_TEST_INTERVAL (SV_NAME_F1 (exp2), -0x1p23, -inf, 50000)
PL_TEST_INTERVAL (SV_NAME_F1 (exp2), -0, ScaleThres, 40000)
PL_TEST_INTERVAL (SV_NAME_F1 (exp2), ScaleThres, -1, 50000)
PL_TEST_INTERVAL (SV_NAME_F1 (exp2), -1, ScaleThres, 50000)
PL_TEST_INTERVAL (SV_NAME_F1 (exp2), ScaleThres, -inf, 50000)
