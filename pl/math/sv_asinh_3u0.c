/*
 * Double-precision SVE asinh(x) function.
 *
 * Copyright (c) 2022-2024, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "sv_math.h"
#include "poly_sve_f64.h"
#include "pl_sig.h"
#include "pl_test.h"
#define SV_LOG_INLINE_POLY_ORDER 5
#include "sv_log_inline.h"

#define SignMask (0x8000000000000000)
#define One (0x3ff0000000000000)
#define Thres (0x5fe0000000000000) /* asuint64 (0x1p511).  */

static const struct data
{
  double poly[18];
  struct sv_log_inline_data log_tbl;

} data = {
  /* Polynomial generated using Remez on [2^-26, 1].  */
  .poly
  = { -0x1.55555555554a7p-3, 0x1.3333333326c7p-4, -0x1.6db6db68332e6p-5,
      0x1.f1c71b26fb40dp-6, -0x1.6e8b8b654a621p-6, 0x1.1c4daa9e67871p-6,
      -0x1.c9871d10885afp-7, 0x1.7a16e8d9d2ecfp-7, -0x1.3ddca533e9f54p-7,
      0x1.0becef748dafcp-7, -0x1.b90c7099dd397p-8, 0x1.541f2bb1ffe51p-8,
      -0x1.d217026a669ecp-9, 0x1.0b5c7977aaf7p-9, -0x1.e0f37daef9127p-11,
      0x1.388b5fe542a6p-12, -0x1.021a48685e287p-14, 0x1.93d4ba83d34dap-18 },
  .log_tbl = SV_LOG_CONSTANTS
};

static svfloat64_t NOINLINE
special_case (svfloat64_t x, svfloat64_t y, svbool_t special) SC_ATTR
{
  return sv_call_f64 (asinh, x, y, special);
}

/* Double-precision implementation of SVE asinh(x).
   asinh is very sensitive around 1, so it is impractical to devise a single
   low-cost algorithm which is sufficiently accurate on a wide range of input.
   Instead we use two different algorithms:
   asinh(x) = sign(x) * log(|x| + sqrt(x^2 + 1)      if |x| >= 1
	    = sign(x) * (|x| + |x|^3 * P(x^2))       otherwise
   where log(x) is an optimized log approximation, and P(x) is a polynomial
   shared with the scalar routine. The greatest observed error 2.51 ULP, in
   |x| >= 1:
   _ZGVsMxv_asinh(0x1.170469d024505p+0) got 0x1.e3181c43b0f36p-1
				       want 0x1.e3181c43b0f39p-1.  */
svfloat64_t SV_NAME_D1 (asinh) (svfloat64_t x, const svbool_t pg) SC_ATTR
{
  const struct data *d = ptr_barrier (&data);

  svuint64_t ix = svreinterpret_u64 (x);
  svuint64_t iax = svbic_x (pg, ix, SignMask);
  svuint64_t sign = svand_x (pg, ix, SignMask);
  svfloat64_t ax = svreinterpret_f64 (iax);

  svbool_t ge1 = svcmpge (pg, iax, One);
  svbool_t special = svcmpge (pg, iax, Thres);

  /* Option 1: |x| >= 1.
     Compute asinh(x) according by asinh(x) = log(x + sqrt(x^2 + 1)).  */
  svfloat64_t option_1 = sv_f64 (0);
  if (likely (svptest_any (pg, ge1)))
    {
      svfloat64_t x2 = svmul_x (pg, ax, ax);
      option_1 = sv_log_inline (
	  pg, svadd_x (pg, ax, svsqrt_x (pg, svadd_x (pg, x2, 1))),
	  &d->log_tbl);
    }

  /* Option 2: |x| < 1.
     Compute asinh(x) using a polynomial.
     The largest observed error in this region is 1.51 ULPs:
     _ZGVsMxv_asinh(0x1.fe12bf8c616a2p-1) got 0x1.c1e649ee2681bp-1
					 want 0x1.c1e649ee2681dp-1.  */
  svfloat64_t option_2 = sv_f64 (0);
  if (likely (svptest_any (pg, svnot_z (pg, ge1))))
    {
      svfloat64_t x2 = svmul_x (pg, ax, ax);
      svfloat64_t x4 = svmul_x (pg, x2, x2);
      svfloat64_t p = sv_pw_horner_17_f64_x (pg, x2, x4, d->poly);
      option_2 = svmla_x (pg, ax, p, svmul_x (pg, x2, ax));
    }

  /* Choose the right option for each lane.  */
  svfloat64_t y = svsel (ge1, option_1, option_2);

  if (unlikely (svptest_any (pg, special)))
    return special_case (
	x, svreinterpret_f64 (sveor_x (pg, svreinterpret_u64 (y), sign)),
	special);
  return svreinterpret_f64 (sveor_x (pg, svreinterpret_u64 (y), sign));
}

PL_SIG (SV, D, 1, asinh, -10.0, 10.0)
PL_TEST_ULP (SV_NAME_D1 (asinh), 2.52)
/* Test vector asinh 3 times, with control lane < 1, > 1 and special.
   Ensures the svsel is choosing the right option in all cases.  */
#define SV_ASINH_INTERVAL(lo, hi, n)                                          \
  PL_TEST_SYM_INTERVAL_C (SV_NAME_D1 (asinh), lo, hi, n, 0.5)                 \
  PL_TEST_SYM_INTERVAL_C (SV_NAME_D1 (asinh), lo, hi, n, 2)                   \
  PL_TEST_SYM_INTERVAL_C (SV_NAME_D1 (asinh), lo, hi, n, 0x1p600)
SV_ASINH_INTERVAL (0, 0x1p-26, 50000)
SV_ASINH_INTERVAL (0x1p-26, 1, 50000)
SV_ASINH_INTERVAL (1, 0x1p511, 50000)
SV_ASINH_INTERVAL (0x1p511, inf, 40000)
