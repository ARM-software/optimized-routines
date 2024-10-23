// clang-format off
/*
 * Function entries for mathbench.
 *
 * Copyright (c) 2022-2024, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#define _ZSF1(fun, a, b) F(fun##f, a, b)
#define _ZSD1(f, a, b) D(f, a, b)

#if WANT_SIMD_TESTS

#define _ZVF1(fun, a, b) VNF(_ZGVnN4v_##fun##f, a, b)
#define _ZVD1(f, a, b) VND(_ZGVnN2v_##f, a, b)

#else

#define _ZVF1(f, a, b)
#define _ZVD1(f, a, b)

#endif

#if WANT_SVE_MATH

#define _ZSVF1(fun, a, b) SVF(_ZGVsMxv_##fun##f, a, b)
#define _ZSVD1(f, a, b) SVD(_ZGVsMxv_##f, a, b)

#else

#define _ZSVF1(f, a, b)
#define _ZSVD1(f, a, b)

#endif

/* No auto-generated wrappers for binary functions - they have be
   manually defined in mathbench_wrappers.h. We have to define silent
   macros for them anyway as they will be emitted by TEST_SIG.  */
#define _ZSF2(...)
#define _ZSD2(...)
#define _ZVF2(...)
#define _ZVD2(...)
#define _ZSVF2(...)
#define _ZSVD2(...)

#include "mathbench_funcs_gen.h"

/* TEST_SIG only emits entries for unary functions, since if a function
   needs to be wrapped in mathbench there is no way for it to know the
   same of the wrapper. Add entries for binary functions, or any other
   exotic signatures that need wrapping, below.  */

{"atan2f", 'f', 0, -10.0, 10.0, {.f = atan2f_wrap}},
{"atan2",  'd', 0, -10.0, 10.0, {.d = atan2_wrap}},
{"powi",   'd', 0,  0.01, 11.1, {.d = powi_wrap}},
{"sincospif", 'f', 0, -3.1, 3.1, {.f = sincospif_wrap}},
{"sincospi", 'd', 0, -3.1, 3.1, {.d = sincospi_wrap}},

#if __linux__
{"_ZGVnN4vl4_modff", 'f', 'n', -10.0, 10.0, {.vnf = _Z_modff_wrap}},
{"_ZGVnN2vl8_modf",  'd', 'n', -10.0, 10.0, {.vnd = _Z_modf_wrap}},
{"_ZGVnN4vl4l4_sincosf", 'f', 'n', -3.1, 3.1, {.vnf = _Z_sincosf_wrap}},
{"_ZGVnN2vl8l8_sincos", 'd', 'n', -3.1, 3.1, {.vnd = _Z_sincos_wrap}},
{"_ZGVnN4vl4l4_sincospif", 'f', 'n', -3.1, 3.1, {.vnf = _Z_sincospif_wrap}},
{"_ZGVnN2vl8l8_sincospi", 'd', 'n', -3.1, 3.1, {.vnd = _Z_sincospi_wrap}},
{"_ZGVnN4v_cexpif", 'f', 'n', -3.1, 3.1, {.vnf = _Z_cexpif_wrap}},
{"_ZGVnN2v_cexpi", 'd', 'n', -3.1, 3.1, {.vnd = _Z_cexpi_wrap}},

# if WANT_SVE_MATH
{"_ZGVsMxvl4_modff", 'f', 's', -10.0, 10.0, {.svf = _Z_sv_modff_wrap}},
{"_ZGVsMxvl8_modf",  'd', 's', -10.0, 10.0, {.svd = _Z_sv_modf_wrap}},
{"_ZGVsMxvv_powi",   'f', 's', -10.0, 10.0, {.svf = _Z_sv_powi_wrap}},
{"_ZGVsMxvv_powk",   'd', 's', -10.0, 10.0, {.svd = _Z_sv_powk_wrap}},
{"_ZGVsMxvl4l4_sincosf", 'f', 's', -3.1, 3.1, {.svf = _Z_sv_sincosf_wrap}},
{"_ZGVsMxvl8l8_sincos", 'd', 's', -3.1, 3.1, {.svd = _Z_sv_sincos_wrap}},
{"_ZGVsMxvl4l4_sincospif", 'f', 's', -3.1, 3.1, {.svf = _Z_sv_sincospif_wrap}},
{"_ZGVsMxvl8l8_sincospi", 'd', 's', -3.1, 3.1, {.svd = _Z_sv_sincospi_wrap}},
{"_ZGVsMxv_cexpif", 'f', 's', -3.1, 3.1, {.svf = _Z_sv_cexpif_wrap}},
{"_ZGVsMxv_cexpi", 'd', 's', -3.1, 3.1, {.svd = _Z_sv_cexpi_wrap}},
# endif
#endif
    // clang-format on
