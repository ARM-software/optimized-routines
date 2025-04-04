/*
 * Single-precision SVE 10^x function.
 *
 * Copyright (c) 2023-2025, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#define _GNU_SOURCE
#include "sv_math.h"
#include "test_sig.h"
#include "test_defs.h"

/* For x < -Thres (-log10(2^126)), the result is subnormal and not handled
   correctly by FEXPA.  */
#define Thres 0x1.2f702p+5

static const struct data
{
  float log10_2, log2_10_hi, log2_10_lo, c1;
  float c0, shift, thres;
} data = {
  /* Coefficients generated using Remez algorithm with minimisation of relative
     error.  */
  .c0 = 0x1.26bb62p1,
  .c1 = 0x1.53524cp1,
  /* 1.5*2^17 + 127, a shift value suitable for FEXPA.  */
  .shift = 0x1.803f8p17f,
  .log10_2 = 0x1.a934fp+1,
  .log2_10_hi = 0x1.344136p-2,
  .log2_10_lo = -0x1.ec10cp-27,
  .thres = Thres,
};

static inline svfloat32_t
sv_exp10f_inline (svfloat32_t x, const svbool_t pg, const struct data *d)
{
  /* exp10(x) = 2^(n/N) * 10^r = 2^n * (1 + poly (r)),
     with poly(r) in [1/sqrt(2), sqrt(2)] and
     x = r + n * log10(2) / N, with r in [-log10(2)/2N, log10(2)/2N].  */

  svfloat32_t lane_consts = svld1rq (pg, &d->log10_2);

  /* n = round(x/(log10(2)/N)).  */
  svfloat32_t shift = sv_f32 (d->shift);
  svfloat32_t z = svmla_lane (shift, x, lane_consts, 0);
  svfloat32_t n = svsub_x (pg, z, shift);

  /* r = x - n*log10(2)/N.  */
  svfloat32_t r = x;
  r = svmls_lane (r, n, lane_consts, 1);
  r = svmls_lane (r, n, lane_consts, 2);

  svfloat32_t scale = svexpa (svreinterpret_u32 (z));

  /* Polynomial evaluation: poly(r) ~ exp10(r)-1.  */
  svfloat32_t poly = svmla_lane (sv_f32 (d->c0), r, lane_consts, 3);
  poly = svmul_x (pg, poly, r);
  return svmla_x (pg, scale, scale, poly);
}

static svfloat32_t NOINLINE
special_case (svfloat32_t x, svbool_t special, const struct data *d)
{
  return sv_call_f32 (exp10f, x, sv_exp10f_inline (x, svptrue_b32 (), d),
		      special);
}

/* Single-precision SVE exp10f routine. Based on the FEXPA instruction.
   Worst case error is 1.10 ULP.
   _ZGVsMxv_exp10f (0x1.cc76dep+3) got 0x1.be0172p+47
				  want 0x1.be017p+47.  */
svfloat32_t SV_NAME_F1 (exp10) (svfloat32_t x, const svbool_t pg)
{
  const struct data *d = ptr_barrier (&data);
  svbool_t special = svacgt (pg, x, d->thres);
  if (unlikely (svptest_any (special, special)))
    return special_case (x, special, d);
  return sv_exp10f_inline (x, pg, d);
}

#if WANT_EXP10_TESTS
TEST_SIG (SV, F, 1, exp10, -9.9, 9.9)
TEST_ULP (SV_NAME_F1 (exp10), 0.60)
TEST_DISABLE_FENV (SV_NAME_F1 (exp10))
TEST_SYM_INTERVAL (SV_NAME_F1 (exp10), 0, Thres, 50000)
TEST_SYM_INTERVAL (SV_NAME_F1 (exp10), Thres, inf, 50000)
#endif
CLOSE_SVE_ATTR
