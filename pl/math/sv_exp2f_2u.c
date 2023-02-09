/*
 * Single-precision SVE 2^x function.
 *
 * Copyright (c) 2023, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "sv_math.h"
#include "pl_sig.h"
#include "pl_test.h"
#include "sv_expf_specialcase.h"

#if SV_SUPPORTED

#define C(i) __sv_exp2f_poly[i]
#define ScaleThres (192.0f)

#if SV_EXP2F_USE_FEXPA

#define Shift sv_f32 (0x1.903f8p17f) /* 1.5*2^17 + 127.  */
#define Thres                                                                  \
  (0x1.5d5e2ap+6f) /* Roughly 87.3. For x < -Thres, the result is subnormal    \
		      and not handled correctly by FEXPA.  */

static NOINLINE svfloat32_t
special_case (svfloat32_t x, svfloat32_t y, svbool_t special)
{
  /* The special-case handler from the Neon routine does not handle subnormals
     in a way that is compatible with FEXPA. For the FEXPA variant we just fall
     back to scalar exp2f.  */
  return sv_call_f32 (exp2f, x, y, special);
}

#else

#define Shift sv_f32 (0x1.8p23f) /* 1.5 * 2^23.  */
#define Thres (126.0f)

#endif
/* Single-precision SVE exp2f routine. Implements the Neon algorithm
   for exp2f from math/.

   Worst case error with FEXPA enabled is 1.04 ULPs.
   __sv_exp2f(0x1.943b9p-1) got 0x1.ba7eb2p+0
	  want 0x1.ba7ebp+0

   Worst case error without FEXPA is 1.96 ULPs.
   __sv_exp2f(0x1.ff7338p-2) got 0x1.69e764p+0
			    want 0x1.69e768p+0.  */
svfloat32_t
__sv_exp2f_x (svfloat32_t x, const svbool_t pg)
{
  /* exp2(x) = 2^n (1 + poly(r)), with 1 + poly(r) in [1/sqrt(2),sqrt(2)]
    x = n + r, with r in [-1/2, 1/2].  */
  svfloat32_t z = svadd_f32_x (pg, x, Shift);
  svfloat32_t n = svsub_f32_x (pg, z, Shift);
  svfloat32_t r = svsub_f32_x (pg, x, n);

#if SV_EXP2F_USE_FEXPA
  /* NaNs also need special handling with FEXPA.  */
  svbool_t is_special_case
    = svorr_b_z (pg, svacgt_n_f32 (pg, x, Thres), svcmpne_f32 (pg, x, x));
  svfloat32_t scale = svexpa_f32 (sv_as_u32_f32 (z));
#else
  svuint32_t e
    = svlsl_n_u32_x (pg, sv_as_u32_s32 (sv_to_s32_f32_x (pg, n)), 23);
  svbool_t is_special_case = svacgt_n_f32 (pg, n, Thres);
  svfloat32_t scale = sv_as_f32_u32 (svadd_n_u32_x (pg, e, 0x3f800000));
#endif

  /* Polynomial evaluation: poly(r) ~ exp(r)-1.  */
  svfloat32_t r2 = svmul_f32_x (pg, r, r);
  svfloat32_t p = sv_fma_n_f32_x (pg, C (0), r, sv_f32 (C (1)));
  svfloat32_t q = sv_fma_n_f32_x (pg, C (2), r, sv_f32 (C (3)));
  q = sv_fma_f32_x (pg, p, r2, q);
  p = svmul_n_f32_x (pg, r, C (4));
  svfloat32_t poly = sv_fma_f32_x (pg, q, r2, p);

  if (unlikely (svptest_any (pg, is_special_case)))
    {
#if SV_EXP2F_USE_FEXPA
      return special_case (x, sv_fma_f32_x (pg, poly, scale, scale),
			   is_special_case);

#else
      return __sv_expf_specialcase (pg, poly, n, e, is_special_case, scale);

#endif
    }
  return sv_fma_f32_x (pg, poly, scale, scale);
}

PL_ALIAS (__sv_exp2f_x, _ZGVsMxv_exp2f)

PL_SIG (SV, F, 1, exp2, -9.9, 9.9)
PL_TEST_ULP (__sv_exp2f, 1.47)
PL_TEST_INTERVAL (__sv_exp2f, 0, Thres, 40000)
PL_TEST_INTERVAL (__sv_exp2f, Thres, 1, 50000)
PL_TEST_INTERVAL (__sv_exp2f, 1, Thres, 50000)
PL_TEST_INTERVAL (__sv_exp2f, Thres, inf, 50000)
PL_TEST_INTERVAL (__sv_exp2f, -0, -0x1p-23, 40000)
PL_TEST_INTERVAL (__sv_exp2f, -0x1p-23, -1, 50000)
PL_TEST_INTERVAL (__sv_exp2f, -1, -0x1p23, 50000)
PL_TEST_INTERVAL (__sv_exp2f, -0x1p23, -inf, 50000)
PL_TEST_INTERVAL (__sv_exp2f, -0, ScaleThres, 40000)
PL_TEST_INTERVAL (__sv_exp2f, ScaleThres, -1, 50000)
PL_TEST_INTERVAL (__sv_exp2f, -1, ScaleThres, 50000)
PL_TEST_INTERVAL (__sv_exp2f, ScaleThres, -inf, 50000)

#endif
