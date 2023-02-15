/*
 * Double-precision SVE log(x) function.
 *
 * Copyright (c) 2020-2023, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "sv_math.h"
#include "pl_sig.h"
#include "pl_test.h"

#if SV_SUPPORTED

#define A(i) __sv_log_data.poly[i]
#define Ln2 (0x1.62e42fefa39efp-1)
#define N (1 << SV_LOG_TABLE_BITS)
#define OFF (0x3fe6900900000000)

double
optr_aor_log_f64 (double);

static NOINLINE svfloat64_t
__sv_log_specialcase (svfloat64_t x, svfloat64_t y, svbool_t cmp)
{
  return sv_call_f64 (optr_aor_log_f64, x, y, cmp);
}

/* SVE port of Neon log algorithm from math/.
   Maximum measured error is 2.17 ulp:
   SV_NAME_D1 (log)(0x1.a6129884398a3p+0) got 0x1.ffffff1cca043p-2
				 want 0x1.ffffff1cca045p-2.  */
svfloat64_t SV_NAME_D1 (log) (svfloat64_t x, const svbool_t pg)
{
  svuint64_t ix = svreinterpret_u64_f64 (x);
  svuint64_t top = svlsr_n_u64_x (pg, ix, 48);
  svbool_t cmp = svcmpge_u64 (pg, svsub_n_u64_x (pg, top, 0x0010),
			      sv_u64 (0x7ff0 - 0x0010));

  /* x = 2^k z; where z is in range [OFF,2*OFF) and exact.
     The range is split into N subintervals.
     The ith subinterval contains z and c is near its center.  */
  svuint64_t tmp = svsub_n_u64_x (pg, ix, OFF);
  /* Equivalent to (tmp >> (52 - SV_LOG_TABLE_BITS)) % N, since N is a power
     of 2.  */
  svuint64_t i
    = svand_n_u64_x (pg, svlsr_n_u64_x (pg, tmp, (52 - SV_LOG_TABLE_BITS)),
		     N - 1);
  svint64_t k = svasr_n_s64_x (pg, svreinterpret_s64_u64 (tmp),
			       52); /* Arithmetic shift.  */
  svuint64_t iz = svsub_u64_x (pg, ix, svand_n_u64_x (pg, tmp, 0xfffULL << 52));
  svfloat64_t z = svreinterpret_f64_u64 (iz);
  /* Lookup in 2 global lists (length N).  */
  svfloat64_t invc = svld1_gather_u64index_f64 (pg, __sv_log_data.invc, i);
  svfloat64_t logc = svld1_gather_u64index_f64 (pg, __sv_log_data.logc, i);

  /* log(x) = log1p(z/c-1) + log(c) + k*Ln2.  */
  svfloat64_t r = svmla_f64_x (pg, sv_f64 (-1.0), invc, z);
  svfloat64_t kd = svcvt_f64_s64_x (pg, k);
  /* hi = r + log(c) + k*Ln2.  */
  svfloat64_t hi = svmla_n_f64_x (pg, svadd_f64_x (pg, logc, r), kd, Ln2);
  /* y = r2*(A0 + r*A1 + r2*(A2 + r*A3 + r2*A4)) + hi.  */
  svfloat64_t r2 = svmul_f64_x (pg, r, r);
  svfloat64_t y = svmla_n_f64_x (pg, sv_f64 (A (2)), r, A (3));
  svfloat64_t p = svmla_n_f64_x (pg, sv_f64 (A (0)), r, A (1));
  y = svmla_n_f64_x (pg, y, r2, A (4));
  y = svmla_f64_x (pg, p, r2, y);
  y = svmla_f64_x (pg, hi, r2, y);

  if (unlikely (svptest_any (pg, cmp)))
    return __sv_log_specialcase (x, y, cmp);
  return y;
}

PL_SIG (SV, D, 1, log, 0.01, 11.1)
PL_TEST_ULP (SV_NAME_D1 (log), 1.68)
PL_TEST_INTERVAL (SV_NAME_D1 (log), -0.0, -0x1p126, 100)
PL_TEST_INTERVAL (SV_NAME_D1 (log), 0x1p-149, 0x1p-126, 4000)
PL_TEST_INTERVAL (SV_NAME_D1 (log), 0x1p-126, 0x1p-23, 50000)
PL_TEST_INTERVAL (SV_NAME_D1 (log), 0x1p-23, 1.0, 50000)
PL_TEST_INTERVAL (SV_NAME_D1 (log), 1.0, 100, 50000)
PL_TEST_INTERVAL (SV_NAME_D1 (log), 100, inf, 50000)
#endif // SV_SUPPORTED
