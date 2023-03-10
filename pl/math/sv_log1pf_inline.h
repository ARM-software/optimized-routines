/*
 * Helper for SVE routines which calculate log(1 + x) and do not
 * need special-case handling
 *
 * Copyright (c) 2023, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "v_math.h"
#include "math_config.h"
#include "sv_estrinf.h"

#if SV_SUPPORTED

#define Four 0x40800000
#define Ln2 0x1.62e43p-1f

#define C(i) sv_f32 (__log1pf_data.coeffs[i])

static inline svfloat32_t
eval_poly (svfloat32_t m, svbool_t pg)
{
  svfloat32_t p_12 = svmla_f32_x (pg, C (0), m, C (1));
  svfloat32_t p_78 = svmla_f32_x (pg, C (6), m, C (7));

  svfloat32_t m2 = svmul_f32_x (pg, m, m);
  svfloat32_t p_02 = svmla_f32_x (pg, m, m2, p_12);
  svfloat32_t p_36 = ESTRIN_3_ (pg, m, m2, C, 2);
  svfloat32_t p_79 = svmla_f32_x (pg, p_78, m2, C (8));

  svfloat32_t m4 = svmul_f32_x (pg, m2, m2);
  svfloat32_t p_06 = svmla_f32_x (pg, p_02, m4, p_36);

  return svmla_f32_x (pg, p_06, m4, svmul_f32_x (pg, m4, p_79));
}

static inline svfloat32_t
sv_log1pf_inline (svfloat32_t x, svbool_t pg)
{
  svfloat32_t m = svadd_n_f32_x (pg, x, 1.0f);
  svuint32_t k = svsub_n_u32_x (pg, svreinterpret_u32_f32 (m), 0x3f400000);
  k = svand_n_u32_x (pg, k, 0xff800000);
  svfloat32_t s = svreinterpret_f32_u32 (svsub_u32_x (pg, sv_u32 (Four), k));
  svfloat32_t m_scale
    = svreinterpret_f32_u32 (svsub_u32_x (pg, svreinterpret_u32_f32 (x), k));
  m_scale = svadd_f32_x (pg, m_scale,
			 svmla_f32_x (pg, sv_f32 (-1.0f), sv_f32 (0.25f), s));
  svfloat32_t p = eval_poly (m_scale, pg);
  svfloat32_t scale_back
    = svmul_n_f32_x (pg, svcvt_f32_u32_x (pg, k), 0x1.0p-23f);
  return svmla_n_f32_x (pg, p, scale_back, Ln2);
}

#endif // SV_SUPPORTED
