/*
 * Double-precision SVE log10(x) function.
 *
 * Copyright (c) 2022-2023, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "sv_math.h"
#include "math_config.h"
#include "pl_sig.h"
#include "pl_test.h"

#if SV_SUPPORTED

#define OFF 0x3fe6900900000000
#define N (1 << V_LOG10_TABLE_BITS)

#define A(i) __v_log10_data.poly[i]

static inline svfloat64_t
specialcase (svfloat64_t x, svfloat64_t y, svbool_t special)
{
  return sv_call_f64 (log10, x, y, special);
}

/* SVE log10 algorithm. Maximum measured error is 2.46 ulps.
   SV_NAME_D1 (log10)(0x1.131956cd4b627p+0) got 0x1.fffbdf6eaa669p-6
				   want 0x1.fffbdf6eaa667p-6.  */
svfloat64_t SV_NAME_D1 (log10) (svfloat64_t x, const svbool_t pg)
{
  svuint64_t ix = sv_as_u64_f64 (x);
  svuint64_t top = svlsr_n_u64_x (pg, ix, 48);

  svbool_t is_special_case
    = svcmpge_n_u64 (pg, svsub_n_u64_x (pg, top, 0x0010), 0x07ff0 - 0x0010);

  /* x = 2^k z; where z is in range [OFF,2*OFF) and exact.
     The range is split into N subintervals.
     The ith subinterval contains z and c is near its center.  */
  svuint64_t tmp = svsub_n_u64_x (pg, ix, OFF);
  svuint64_t i
    = sv_mod_n_u64_x (pg, svlsr_n_u64_x (pg, tmp, 52 - V_LOG10_TABLE_BITS), N);
  svfloat64_t k
    = svcvt_f64_s64_x (pg, svasr_n_s64_x (pg, sv_as_s64_u64 (tmp), 52));
  svfloat64_t z = sv_as_f64_u64 (
    svsub_u64_x (pg, ix, svand_n_u64_x (pg, tmp, 0xfffULL << 52)));

  /* log(x) = k*log(2) + log(c) + log(z/c).  */

  svuint64_t idx = svmul_n_u64_x (pg, i, 2);
  svfloat64_t invc
    = svld1_gather_u64index_f64 (pg, &__v_log10_data.tab[0].invc, idx);
  svfloat64_t logc
    = svld1_gather_u64index_f64 (pg, &__v_log10_data.tab[0].log10c, idx);

  /* We approximate log(z/c) with a polynomial P(x) ~= log(x + 1):
     r = z/c - 1 (we look up precomputed 1/c)
     log(z/c) ~= P(r).  */
  svfloat64_t r = sv_fma_f64_x (pg, z, invc, sv_f64 (-1.0));

  /* hi = log(c) + k*log(2).  */
  svfloat64_t w = sv_fma_n_f64_x (pg, __v_log10_data.invln10, r, logc);
  svfloat64_t hi = sv_fma_n_f64_x (pg, __v_log10_data.log10_2, k, w);

  /* y = r2*(A0 + r*A1 + r2*(A2 + r*A3 + r2*A4)) + hi.  */
  svfloat64_t r2 = svmul_f64_x (pg, r, r);
  svfloat64_t y = sv_fma_n_f64_x (pg, A (3), r, sv_f64 (A (2)));
  svfloat64_t p = sv_fma_n_f64_x (pg, A (1), r, sv_f64 (A (0)));
  y = sv_fma_n_f64_x (pg, A (4), r2, y);
  y = sv_fma_f64_x (pg, y, r2, p);
  y = sv_fma_f64_x (pg, y, r2, hi);

  if (unlikely (svptest_any (pg, is_special_case)))
    {
      return specialcase (x, y, is_special_case);
    }
  return y;
}

PL_SIG (SV, D, 1, log10, 0.01, 11.1)
PL_TEST_ULP (SV_NAME_D1 (log10), 1.97)
PL_TEST_INTERVAL (SV_NAME_D1 (log10), -0.0, -0x1p126, 100)
PL_TEST_INTERVAL (SV_NAME_D1 (log10), 0x1p-149, 0x1p-126, 4000)
PL_TEST_INTERVAL (SV_NAME_D1 (log10), 0x1p-126, 0x1p-23, 50000)
PL_TEST_INTERVAL (SV_NAME_D1 (log10), 0x1p-23, 1.0, 50000)
PL_TEST_INTERVAL (SV_NAME_D1 (log10), 1.0, 100, 50000)
PL_TEST_INTERVAL (SV_NAME_D1 (log10), 100, inf, 50000)
#endif
