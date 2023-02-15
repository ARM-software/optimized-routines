/*
 * Single-precision vector e^x function.
 *
 * Copyright (c) 2019-2023, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "sv_math.h"
#include "pl_sig.h"
#include "pl_test.h"
#include "sv_expf_specialcase.h"

#if SV_SUPPORTED

#define C(i) __sv_expf_poly[i]

#define InvLn2 (0x1.715476p+0f)
#define Ln2hi (0x1.62e4p-1f)
#define Ln2lo (0x1.7f7d1cp-20f)

#if SV_EXPF_USE_FEXPA

#define Shift (0x1.903f8p17f) /* 1.5*2^17 + 127.  */
#define Thres                                                                  \
  (0x1.5d5e2ap+6f) /* Roughly 87.3. For x < -Thres, the result is subnormal    \
		      and not handled correctly by FEXPA.  */

static NOINLINE svfloat32_t
special_case (svfloat32_t x, svfloat32_t y, svbool_t special)
{
  /* The special-case handler from the Neon routine does not handle subnormals
     in a way that is compatible with FEXPA. For the FEXPA variant we just fall
     back to scalar expf.  */
  return sv_call_f32 (expf, x, y, special);
}

#else

#define Shift (0x1.8p23f) /* 1.5 * 2^23.  */
#define Thres (126.0f)

#endif

/* Optimised single-precision SVE exp function. By default this is an SVE port
   of the Neon algorithm from math/. Alternatively, enable a modification of
   that algorithm that looks up scale using SVE FEXPA instruction with
   SV_EXPF_USE_FEXPA.

   Worst-case error of the default algorithm is 1.95 ulp:
   SV_NAME_F1 (exp)(-0x1.4cb74ap+2) got 0x1.6a022cp-8
			     want 0x1.6a023p-8.

   Worst-case error when using FEXPA is 1.04 ulp:
   SV_NAME_F1 (exp)(0x1.a8eda4p+1) got 0x1.ba74bcp+4
			   want 0x1.ba74bap+4.  */
svfloat32_t SV_NAME_F1 (exp) (svfloat32_t x, const svbool_t pg)
{
  /* exp(x) = 2^n (1 + poly(r)), with 1 + poly(r) in [1/sqrt(2),sqrt(2)]
     x = ln2*n + r, with r in [-ln2/2, ln2/2].  */

  /* n = round(x/(ln2/N)).  */
  svfloat32_t z = svmla_n_f32_x (pg, sv_f32 (Shift), x, InvLn2);
  svfloat32_t n = svsub_n_f32_x (pg, z, Shift);

  /* r = x - n*ln2/N.  */
  svfloat32_t r = svmla_n_f32_x (pg, x, n, -Ln2hi);
  r = svmla_n_f32_x (pg, r, n, -Ln2lo);

/* scale = 2^(n/N).  */
#if SV_EXPF_USE_FEXPA
  /* NaNs also need special handling with FEXPA.  */
  svbool_t is_special_case
    = svorr_b_z (pg, svacgt_n_f32 (pg, x, Thres), svcmpne_f32 (pg, x, x));
  svfloat32_t scale = svexpa_f32 (sv_as_u32_f32 (z));
#else
  svuint32_t e = svlsl_n_u32_x (pg, sv_as_u32_f32 (z), 23);
  svbool_t is_special_case = svacgt_n_f32 (pg, n, Thres);
  svfloat32_t scale = sv_as_f32_u32 (svadd_n_u32_x (pg, e, 0x3f800000));
#endif

  /* y = exp(r) - 1 ~= r + C1 r^2 + C2 r^3 + C3 r^4.  */
  svfloat32_t r2 = svmul_f32_x (pg, r, r);
  svfloat32_t p = svmla_n_f32_x (pg, sv_f32 (C (1)), r, C (0));
  svfloat32_t q = svmla_n_f32_x (pg, sv_f32 (C (3)), r, C (2));
  q = svmla_f32_x (pg, q, r2, p);
  p = svmul_n_f32_x (pg, r, C (4));
  svfloat32_t poly = svmla_f32_x (pg, p, r2, q);

  if (unlikely (svptest_any (pg, is_special_case)))
#if SV_EXPF_USE_FEXPA
    return special_case (x, svmla_f32_x (pg, scale, scale, poly),
			 is_special_case);
#else
    return __sv_expf_specialcase (pg, poly, n, e, is_special_case, scale);
#endif

  return svmla_f32_x (pg, scale, scale, poly);
}

PL_SIG (SV, F, 1, exp, -9.9, 9.9)
PL_TEST_ULP (SV_NAME_F1 (exp), 1.46)
PL_TEST_INTERVAL (SV_NAME_F1 (exp), 0, 0x1p-23, 40000)
PL_TEST_INTERVAL (SV_NAME_F1 (exp), 0x1p-23, 1, 50000)
PL_TEST_INTERVAL (SV_NAME_F1 (exp), 1, 0x1p23, 50000)
PL_TEST_INTERVAL (SV_NAME_F1 (exp), 0x1p23, inf, 50000)
PL_TEST_INTERVAL (SV_NAME_F1 (exp), -0, -0x1p-23, 40000)
PL_TEST_INTERVAL (SV_NAME_F1 (exp), -0x1p-23, -1, 50000)
PL_TEST_INTERVAL (SV_NAME_F1 (exp), -1, -0x1p23, 50000)
PL_TEST_INTERVAL (SV_NAME_F1 (exp), -0x1p23, -inf, 50000)
#endif // SV_SUPPORTED
