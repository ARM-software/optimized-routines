/*
 * Function entries for ulp.
 *
 * Copyright (c) 2022-2024, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#if defined(__vpcs) && __aarch64__

#define _ZVF1(f) ZVNF1 (f)
#define _ZVD1(f) ZVND1 (f)
#define _ZVF2(f) ZVNF2 (f)
#define _ZVD2(f) ZVND2 (f)

#else

#define _ZVF1(f)
#define _ZVD1(f)
#define _ZVF2(f)
#define _ZVD2(f)

#endif

#if WANT_SVE_MATH

#define _ZSVF1(f) ZSVF1 (f)
#define _ZSVF2(f) ZSVF2 (f)
#define _ZSVD1(f) ZSVD1 (f)
#define _ZSVD2(f) ZSVD2 (f)

#else

#define _ZSVF1(f)
#define _ZSVF2(f)
#define _ZSVD1(f)
#define _ZSVD2(f)

#endif

#define _ZSF1(f) F1 (f)
#define _ZSF2(f) F2 (f)
#define _ZSD1(f) D1 (f)
#define _ZSD2(f) D2 (f)

#include "ulp_funcs_gen.h"

#if WANT_TRIGPI_TESTS
F (sincospif_sin, sincospif_sin, sinpi, mpfr_sinpi, 1, 1, f1, 0)
F (sincospif_cos, sincospif_cos, cospi, mpfr_cospi, 1, 1, f1, 0)

F (sincospi_sin, sincospi_sin, sinpil, mpfr_sinpi, 1, 0, d1, 0)
F (sincospi_cos, sincospi_cos, cospil, mpfr_cospi, 1, 0, d1, 0)
#endif

#if __linux__
# if WANT_TRIGPI_TESTS
F (_ZGVnN4v_sincospif_sin, v_sincospif_sin, sinpi, mpfr_sinpi, 1, 1, f1, 0)
F (_ZGVnN4v_sincospif_cos, v_sincospif_cos, cospi, mpfr_cospi, 1, 1, f1, 0)

F (_ZGVnN2v_sincospi_sin, v_sincospi_sin, sinpil, mpfr_sinpi, 1, 0, d1, 0)
F (_ZGVnN2v_sincospi_cos, v_sincospi_cos, cospil, mpfr_cospi, 1, 0, d1, 0)
# endif

F (_ZGVnN4v_sincosf_sin, v_sincosf_sin, sin, mpfr_sin, 1, 1, f1, 0)
F (_ZGVnN4v_sincosf_cos, v_sincosf_cos, cos, mpfr_cos, 1, 1, f1, 0)
F (_ZGVnN4v_cexpif_sin, v_cexpif_sin, sin, mpfr_sin, 1, 1, f1, 0)
F (_ZGVnN4v_cexpif_cos, v_cexpif_cos, cos, mpfr_cos, 1, 1, f1, 0)
F (_ZGVnN4vl4_modff_frac, v_modff_frac, modf_frac, modf_mpfr_frac, 1, 1, f1, 0)
F (_ZGVnN4vl4_modff_int, v_modff_int, modf_int, modf_mpfr_int, 1, 1, f1, 0)

F (_ZGVnN2v_sincos_sin, v_sincos_sin, sinl, mpfr_sin, 1, 0, d1, 0)
F (_ZGVnN2v_sincos_cos, v_sincos_cos, cosl, mpfr_cos, 1, 0, d1, 0)
F (_ZGVnN2v_cexpi_sin, v_cexpi_sin, sinl, mpfr_sin, 1, 0, d1, 0)
F (_ZGVnN2v_cexpi_cos, v_cexpi_cos, cosl, mpfr_cos, 1, 0, d1, 0)
F (_ZGVnN2vl8_modf_frac, v_modf_frac, modfl_frac, modf_mpfr_frac, 1, 0, d1, 0)
F (_ZGVnN2vl8_modf_int, v_modf_int, modfl_int, modf_mpfr_int, 1, 0, d1, 0)

# if WANT_SVE_MATH
#  if !USE_MPFR
SVF (_ZGVsMxvv_powk, Z_sv_powk, ref_powi, mpfr_powi, 2, 0, d2, 0)
SVF (_ZGVsMxvv_powi, Z_sv_powi, ref_powif, mpfr_powi, 2, 1, f2, 0)
#  endif

SVF (_ZGVsMxv_sincosf_sin, sv_sincosf_sin, sin, mpfr_sin, 1, 1, f1, 0)
SVF (_ZGVsMxv_sincosf_cos, sv_sincosf_cos, cos, mpfr_cos, 1, 1, f1, 0)
SVF (_ZGVsMxv_cexpif_sin, sv_cexpif_sin, sin, mpfr_sin, 1, 1, f1, 0)
SVF (_ZGVsMxv_cexpif_cos, sv_cexpif_cos, cos, mpfr_cos, 1, 1, f1, 0)
SVF (_ZGVsMxvl4_modff_frac, sv_modff_frac, modf_frac, modf_mpfr_frac, 1, 1, f1,
     0)
SVF (_ZGVsMxvl4_modff_int, sv_modff_int, modf_int, modf_mpfr_int, 1, 1, f1, 0)

SVF (_ZGVsMxv_sincos_sin, sv_sincos_sin, sinl, mpfr_sin, 1, 0, d1, 0)
SVF (_ZGVsMxv_sincos_cos, sv_sincos_cos, cosl, mpfr_cos, 1, 0, d1, 0)
SVF (_ZGVsMxv_cexpi_sin, sv_cexpi_sin, sinl, mpfr_sin, 1, 0, d1, 0)
SVF (_ZGVsMxv_cexpi_cos, sv_cexpi_cos, cosl, mpfr_cos, 1, 0, d1, 0)
SVF (_ZGVsMxvl8_modf_frac, sv_modf_frac, modfl_frac, modf_mpfr_frac, 1, 0, d1,
     0)
SVF (_ZGVsMxvl8_modf_int, sv_modf_int, modfl_int, modf_mpfr_int, 1, 0, d1, 0)

#  if WANT_TRIGPI_TESTS
SVF (_ZGVsMxvl4l4_sincospif_sin, sv_sincospif_sin, sinpi, mpfr_sinpi, 1, 1, f1,
     0)
SVF (_ZGVsMxvl4l4_sincospif_cos, sv_sincospif_cos, cospi, mpfr_cospi, 1, 1, f1,
     0)
SVF (_ZGVsMxvl8l8_sincospi_sin, sv_sincospi_sin, sinpil, mpfr_sinpi, 1, 0, d1,
     0)
SVF (_ZGVsMxvl8l8_sincospi_cos, sv_sincospi_cos, cospil, mpfr_cospi, 1, 0, d1,
     0)
#  endif
# endif
#endif
