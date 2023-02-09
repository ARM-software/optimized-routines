/*
 * Double-precision vector e^x function.
 *
 * Copyright (c) 2023, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "sv_math.h"
#include "pl_sig.h"
#include "pl_test.h"

#if SV_SUPPORTED

#define C(i) __sv_exp_poly[i]

#define InvLn2 (0x1.71547652b82fep+0) /* 1/ln2.  */
#define Ln2hi (0x1.62e42fefa3800p-1)
#define Ln2lo (0x1.ef35793c76730p-45)

#define Shift                                                                  \
  (double) (0x1.800000000ffc0p+46) /* 1.5*2^46+1023. This value is further     \
				      explained below.  */
#define Thres (704.0)

/* Update of both special and non-special cases, if any special case is
   detected.  */
static inline svfloat64_t
specialcase (svbool_t pg, svfloat64_t s, svfloat64_t y, svfloat64_t n)
{
  svfloat64_t absn = svabs_f64_x (pg, n);

  /* s=2^n may overflow, break it up into s=s1*s2,
     such that exp = s + s*y can be computed as s1*(s2+s2*y)
     and s1*s1 overflows only if n>0.  */

  /* If n<=0 then set b to 0x6, 0 otherwise.  */
  svbool_t p_sign = svcmple_n_f64 (pg, n, 0.0); /* n <= 0.  */
  svuint64_t b
    = svdup_n_u64_z (p_sign,
		     0x6000000000000000); /* Inactive lanes set to 0.  */

  /* Set s1 to generate overflow depending on sign of exponent n.  */
  svfloat64_t s1 = sv_as_f64_u64 (
    svsubr_n_u64_x (pg, b, 0x7000000000000000)); /* 0x70...0 - b.  */
  /* Offset s to avoid overflow in final result if n is below threshold.  */
  svfloat64_t s2 = sv_as_f64_u64 (
    svadd_u64_x (pg, svsub_n_u64_x (pg, sv_as_u64_f64 (s), 0x3010000000000000),
		 b)); /* as_u64 (s) - 0x3010...0 + b.  */

  /* |n| > 1280 => 2^(n) overflows.  */
  svbool_t p_cmp = svcmpgt_n_f64 (pg, absn, 1280.0);

  svfloat64_t r1 = svmul_f64_x (pg, s1, s1);
  svfloat64_t r2 = sv_fma_f64_x (pg, y, s2, s2);
  svfloat64_t r0 = svmul_f64_x (pg, r2, s1);

  return svsel_f64 (p_cmp, r1, r0);
}

/* SVE exp algorithm. Maximum measured error is 1.01ulps:
   SV_NAME_D1 (exp)(0x1.4619d7b04da41p+6) got 0x1.885d9acc41da7p+117
				 want 0x1.885d9acc41da6p+117.  */
svfloat64_t SV_NAME_D1 (exp) (svfloat64_t x, const svbool_t pg)
{
  svbool_t special = svacgt_n_f64 (pg, x, Thres);
  svbool_t isnan = svcmpne_f64 (pg, x, x);

  /* Use a modifed version of the shift used for flooring, such that x/ln2 is
     rounded to a multiple of 2^-6=1/64, shift = 1.5 * 2^52 * 2^-6 = 1.5 * 2^46.

     n is not an integer but can be written as n = m + i/64, with i and m
     integer, 0 <= i < 64 and m <= n.

     Bits 5:0 of z will be null every time x/ln2 reaches a new integer value
     (n=m, i=0), and is incremented every time z (or n) is incremented by 1/64.
     FEXPA expects i in bits 5:0 of the input so it can be used as index into
     FEXPA hardwired table T[i] = 2^(i/64) for i = 0:63, that will in turn
     populate the mantissa of the output. Therefore, we use u=asuint(z) as input
     to FEXPA.

     We add 1023 to the modified shift value in order to set bits 16:6 of u to
     1, such that once these bits are moved to the exponent of the output of
     FEXPA, we get the exponent of 2^n right, i.e. we get 2^m.  */
  svfloat64_t z = sv_fma_n_f64_x (pg, InvLn2, x, sv_f64 (Shift));
  svuint64_t u = sv_as_u64_f64 (z);
  svfloat64_t n = svsub_n_f64_x (pg, z, Shift);

  /* r = x - n * ln2, r is in [-ln2/(2N), ln2/(2N)].  */
  svfloat64_t r = sv_fma_n_f64_x (pg, -Ln2hi, n, x);
  r = sv_fma_n_f64_x (pg, -Ln2lo, n, r);

  /* y = exp(r) - 1 ~= r + C0 r^2 + C1 r^3 + C2 r^4 + C3 r^5.  */
  svfloat64_t r2 = svmul_f64_x (pg, r, r);
  svfloat64_t c2_c3r = sv_fma_n_f64_x (pg, C (3), r, sv_f64 (C (2)));
  svfloat64_t c0_c1r = sv_fma_n_f64_x (pg, C (1), r, sv_f64 (C (0)));
  svfloat64_t c0_to_3 = sv_fma_f64_x (pg, r2, c2_c3r, c0_c1r);
  svfloat64_t y = sv_fma_f64_x (pg, r2, c0_to_3, r);

  /* s = 2^n, computed using FEXPA. FEXPA does not propagate NaNs, so for
     consistent NaN handling we have to manually propagate them. This comes at
     significant performance cost.  */
  svfloat64_t s = svexpa_f64 (u);
  s = svsel_f64 (isnan, x, s);

  /* Assemble result as exp(x) = 2^n * exp(r).  If |x| > Thresh the
     multiplication may overflow, so use special case routine.  */

  if (unlikely (svptest_any (pg, special)))
    {
      /* FEXPA zeroes the sign bit, however the sign is meaningful to the
	 special case function so needs to be copied.
	 e = sign bit of u << 46.  */
      svuint64_t e
	= svand_n_u64_x (pg, svlsl_n_u64_x (pg, u, 46), 0x8000000000000000);
      /* Copy sign to s.  */
      s = sv_as_f64_u64 (svadd_u64_x (pg, e, sv_as_u64_f64 (s)));
      return specialcase (pg, s, y, n);
    }

  /* No special case.  */
  return sv_fma_f64_x (pg, y, s, s);
}

PL_SIG (SV, D, 1, exp, -9.9, 9.9)
PL_TEST_ULP (SV_NAME_D1 (exp), 1.46)
PL_TEST_INTERVAL (SV_NAME_D1 (exp), 0, 0x1p-23, 40000)
PL_TEST_INTERVAL (SV_NAME_D1 (exp), 0x1p-23, 1, 50000)
PL_TEST_INTERVAL (SV_NAME_D1 (exp), 1, 0x1p23, 50000)
PL_TEST_INTERVAL (SV_NAME_D1 (exp), 0x1p23, inf, 50000)
PL_TEST_INTERVAL (SV_NAME_D1 (exp), -0, -0x1p-23, 40000)
PL_TEST_INTERVAL (SV_NAME_D1 (exp), -0x1p-23, -1, 50000)
PL_TEST_INTERVAL (SV_NAME_D1 (exp), -1, -0x1p23, 50000)
PL_TEST_INTERVAL (SV_NAME_D1 (exp), -0x1p23, -inf, 50000)

#endif // SV_SUPPORTED
