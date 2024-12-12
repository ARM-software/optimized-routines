/*
 * Function entries for ulp.
 *
 * Copyright (c) 2022-2024, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */
/* clang-format off */
 F (sincosf_sinf, sincosf_sinf, sincos_sin, sincos_mpfr_sin, 1, 1, f1, 0)
 F (sincosf_cosf, sincosf_cosf, sincos_cos, sincos_mpfr_cos, 1, 1, f1, 0)
 F2 (pow)
 D2 (pow)
#if WANT_SIMD_TESTS
 F (_ZGVnN4v_expf_1u, Z_expf_1u, exp, mpfr_exp, 1, 1, f1, 1)
 F (_ZGVnN4v_exp2f_1u, Z_exp2f_1u, exp2, mpfr_exp2, 1, 1, f1, 1)
 F (_ZGVnN4vv_powf, Z_powf, pow, mpfr_pow, 2, 1, f2, 1)
 F (_ZGVnN2vv_pow, Z_pow, powl, mpfr_pow, 2, 0, d2, 1)
#endif

#if WANT_TRIGPI_TESTS
 F (arm_math_cospif, arm_math_cospif, arm_math_cospi, mpfr_cospi, 1, 1, f1, 0)
 F (arm_math_cospi,  arm_math_cospi,  arm_math_cospil, mpfr_cospi, 1, 0, d1, 0)
 F (arm_math_sinpif, arm_math_sinpif, arm_math_sinpi, mpfr_sinpi, 1, 1, f1, 0)
 F (arm_math_sinpi,  arm_math_sinpi,  arm_math_sinpil, mpfr_sinpi, 1, 0, d1, 0)
 F (arm_math_tanpif, arm_math_tanpif, arm_math_tanpi, mpfr_tanpi, 1, 1, f1, 0)
 F (arm_math_tanpi,  arm_math_tanpi,  arm_math_tanpil, mpfr_tanpi, 1, 0, d1, 0)
 F (arm_math_sincospif_sin, arm_math_sincospif_sin, arm_math_sinpi, mpfr_sinpi, 1, 1, f1, 0)
 F (arm_math_sincospif_cos, arm_math_sincospif_cos, arm_math_cospi, mpfr_cospi, 1, 1, f1, 0)
 F (arm_math_sincospi_sin, arm_math_sincospi_sin, arm_math_sinpil, mpfr_sinpi, 1, 0, d1, 0)
 F (arm_math_sincospi_cos, arm_math_sincospi_cos, arm_math_cospil, mpfr_cospi, 1, 0, d1, 0)
# if WANT_SIMD_TESTS
 F (_ZGVnN4v_cospif, Z_cospif, arm_math_cospi,  mpfr_cospi, 1, 1, f1, 0)
 F (_ZGVnN2v_cospi,  Z_cospi,  arm_math_cospil, mpfr_cospi, 1, 0, d1, 0)
 F (_ZGVnN4v_sinpif, Z_sinpif, arm_math_sinpi,  mpfr_sinpi, 1, 1, f1, 0)
 F (_ZGVnN2v_sinpi,  Z_sinpi,  arm_math_sinpil, mpfr_sinpi, 1, 0, d1, 0)
 F (_ZGVnN4v_tanpif, Z_tanpif, arm_math_tanpi,  mpfr_tanpi, 1, 1, f1, 0)
 F (_ZGVnN2v_tanpi,  Z_tanpi,  arm_math_tanpil, mpfr_tanpi, 1, 0, d1, 0)
 F (_ZGVnN4v_sincospif_sin, v_sincospif_sin, arm_math_sinpi, mpfr_sinpi, 1, 1, f1, 0)
 F (_ZGVnN4v_sincospif_cos, v_sincospif_cos, arm_math_cospi, mpfr_cospi, 1, 1, f1, 0)
 F (_ZGVnN2v_sincospi_sin, v_sincospi_sin, arm_math_sinpil, mpfr_sinpi, 1, 0, d1, 0)
 F (_ZGVnN2v_sincospi_cos, v_sincospi_cos, arm_math_cospil, mpfr_cospi, 1, 0, d1, 0)
# endif
# if WANT_SVE_MATH
 SVF (_ZGVsMxv_cospif, Z_sv_cospif, arm_math_cospi,  mpfr_cospi, 1, 1, f1, 0)
 SVF (_ZGVsMxv_cospi,  Z_sv_cospi,  arm_math_cospil, mpfr_cospi, 1, 0, d1, 0)
 SVF (_ZGVsMxv_sinpif, Z_sv_sinpif, arm_math_sinpi,  mpfr_sinpi, 1, 1, f1, 0)
 SVF (_ZGVsMxv_sinpi,  Z_sv_sinpi,  arm_math_sinpil, mpfr_sinpi, 1, 0, d1, 0)
 SVF (_ZGVsMxv_tanpif, Z_sv_tanpif, arm_math_tanpi,  mpfr_tanpi, 1, 1, f1, 0)
 SVF (_ZGVsMxv_tanpi,  Z_sv_tanpi,  arm_math_tanpil, mpfr_tanpi, 1, 0, d1, 0)
 SVF (_ZGVsMxvl4l4_sincospif_sin, sv_sincospif_sin, arm_math_sinpi, mpfr_sinpi, 1, 1, f1, 0)
 SVF (_ZGVsMxvl4l4_sincospif_cos, sv_sincospif_cos, arm_math_cospi, mpfr_cospi, 1, 1, f1, 0)
 SVF (_ZGVsMxvl8l8_sincospi_sin, sv_sincospi_sin, arm_math_sinpil, mpfr_sinpi, 1, 0, d1, 0)
 SVF (_ZGVsMxvl8l8_sincospi_cos, sv_sincospi_cos, arm_math_cospil, mpfr_cospi, 1, 0, d1, 0)
# endif
#endif

 /* clang-format on */

#define _ZSF1(f) F1 (f)
#define _ZSF2(f) F2 (f)
#define _ZSD1(f) D1 (f)
#define _ZSD2(f) D2 (f)

#define _ZVF1(f) ZVNF1 (f)
#define _ZVD1(f) ZVND1 (f)
#define _ZVF2(f) ZVNF2 (f)
#define _ZVD2(f) ZVND2 (f)

#define _ZSVF1(f) ZSVF1 (f)
#define _ZSVF2(f) ZSVF2 (f)
#define _ZSVD1(f) ZSVD1 (f)
#define _ZSVD2(f) ZSVD2 (f)

#include "ulp_funcs_gen.h"
