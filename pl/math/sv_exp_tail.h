/*
 * Double-precision SVE e^(x+tail) function.
 *
 * Copyright (c) 2021-2023, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#ifndef SV_EXP_TAIL_H
#define SV_EXP_TAIL_H

#include "sv_math.h"

#include "v_exp_tail.h"

#define C1 sv_f64 (C1_scal)
#define C2 sv_f64 (C2_scal)
#define C3 sv_f64 (C3_scal)
#define MinusLn2hi (-Ln2hi_scal)
#define MinusLn2lo (-Ln2lo_scal)

#define N (1 << V_EXP_TAIL_TABLE_BITS)
#define Tab __v_exp_tail_data
#define IndexMask (N - 1)
#define Shift sv_f64 (0x1.8p+52)
#define Thres 704.0

static inline svfloat64_t
sv_exp_tail_special_case (svbool_t pg, svfloat64_t s, svfloat64_t y,
			  svfloat64_t n)
{
  svfloat64_t absn = svabs_x (pg, n);

  /* 2^(n/N) may overflow, break it up into s1*s2.  */
  svuint64_t b
      = svsel (svcmple (pg, n, 0), sv_u64 (0x6000000000000000), sv_u64 (0));
  svfloat64_t s1 = svreinterpret_f64 (svsubr_x (pg, b, 0x7000000000000000));
  svfloat64_t s2 = svreinterpret_f64 (svadd_x (
      pg, svsub_x (pg, svreinterpret_u64 (s), 0x3010000000000000), b));

  svbool_t cmp = svcmpgt (pg, absn, 1280.0 * N);
  svfloat64_t r1 = svmul_x (pg, s1, s1);
  svfloat64_t r0 = svmul_x (pg, svmla_x (pg, s2, s2, y), s1);
  return svsel (cmp, r1, r0);
}

static inline svfloat64_t
sv_exp_tail (const svbool_t pg, svfloat64_t x, svfloat64_t xtail)
{
  /* Calculate exp(x + xtail).  */
  svfloat64_t z = svmla_x (pg, Shift, x, InvLn2_scal);
  svfloat64_t n = svsub_x (pg, z, Shift);

  svfloat64_t r = svmla_x (pg, x, n, MinusLn2hi);
  r = svmla_x (pg, r, n, MinusLn2lo);

  svuint64_t u = svreinterpret_u64 (z);
  svuint64_t e = svlsl_x (pg, u, 52 - V_EXP_TAIL_TABLE_BITS);
  svuint64_t i = svand_x (pg, u, IndexMask);

  svfloat64_t y = svmla_x (pg, C2, r, C3);
  y = svmla_x (pg, C1, r, y);
  y = svmla_x (pg, sv_f64 (1.0), r, y);
  y = svmla_x (pg, xtail, r, y);

  /* s = 2^(n/N).  */
  u = svld1_gather_index (pg, Tab, i);
  svfloat64_t s = svreinterpret_f64 (svadd_x (pg, u, e));

  svbool_t cmp = svcmpgt (pg, svabs_x (pg, x), Thres);
  if (unlikely (svptest_any (pg, cmp)))
    {
      return sv_exp_tail_special_case (pg, s, y, n);
    }
  return svmla_x (pg, s, s, y);
}

#endif
