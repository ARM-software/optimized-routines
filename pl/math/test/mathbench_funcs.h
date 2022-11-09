// clang-format off
/*
 * Function entries for mathbench.
 *
 * Copyright (c) 2022, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */
F (acoshf, 1.0, 10.0)
F (asinhf, -10.0, 10.0)
F (atanf, -10.0, 10.0)
{"atan2f", 'f', 0, -10.0, 10.0, {.f = atan2f_wrap}},
F (cosf, -3.1, 3.1)
F (coshf, -10.0, 10.0)
F (erfcf, -4.0, 10.0)
F (erff, -4.0, 4.0)
F (expm1f, -9.9, 9.9)
F (log10f, 0.01, 11.1)
F (log1pf, -0.9, 10.0)
F (log2f, 0.01, 11.1)
F (sinf, -3.1, 3.1)
F (sinhf, -10.0, 10.0)
F (tanf, -3.1, 3.1)

D (acosh, 1.0, 10.0)
D (asinh, -10.0, 10.0)
D (atan, -10.0, 10.0)
{"atan2", 'd', 0, -10.0, 10.0, {.d = atan2_wrap}},
D (cos, -3.1, 3.1)
D (erf, -6,6)
D (erfc, -6.0, 28.0)
D (expm1, -9.9, 9.9)
D (log10, 0.01, 11.1)
D (log1p, -0.9, 10.0)
D (log2, 0.01, 11.1)
{"powi", 'd', 0, 0.01, 11.1, {.d = powi_wrap}},
D (sin, -3.1, 3.1)

#if WANT_VMATH
F (__s_asinhf, -10.0, 10.0)
F (__s_atanf, -10.0, 10.0)
D (__s_atan, -10.0, 10.0)
{"__s_atan2f", 'f', 0, -10.0, 10.0, {.f = __s_atan2f_wrap}},
{"__s_atan2", 'd', 0, -10.0, 10.0, {.d = __s_atan2_wrap}},
F (__s_coshf, -10.0, 10.0)
F (__s_erff, -4.0, 4.0)
D (__s_erf, -6.0, 6.0)
F (__s_erfcf, -6.0, 28.0)
D (__s_erfc, -6.0, 28.0)
F (__s_expm1f, -9.9, 9.9)
F (__s_log10f, 0.01, 11.1)
D (__s_log10, 0.01, 11.1)
F (__s_log1pf, -0.9, 10.0)
D (__s_log1p, -0.9, 10.0)
F (__s_log2f, 0.01, 11.1)
D (__s_log2, 0.01, 11.1)
F (__s_sinhf, -10.0, 10.0)
F (__s_tanf, -3.1, 3.1)
#if __aarch64__
VF (__v_asinhf, -10.0, 10.0)
VF (__v_atanf, -10.0, 10.0)
VD (__v_atan, -10.0, 10.0)
{"__v_atan2f", 'f', 'v', -10.0, 10.0, {.vf = __v_atan2f_wrap}},
{"__v_atan2", 'd', 'v', -10.0, 10.0, {.vd = __v_atan2_wrap}},
VF (__v_coshf, -10.0, 10.0)
VF  (__v_erff, -4.0, 4.0)
VD  (__v_erf, -6.0, 6.0)
VF (__v_erfcf, -6.0, 28.0)
VD (__v_erfc, -6.0, 28.0)
VF (__v_expm1f, -9.9, 9.9)
VD (__v_log10, 0.01, 11.1)
VF (__v_log10f, 0.01, 11.1)
VF (__v_log1pf, -0.9, 10.0)
VD (__v_log1p, -0.9, 10.0)
VF (__v_log2f, 0.01, 11.1)
VD (__v_log2, 0.01, 11.1)
VF (__v_sinhf, -10.0, 10.0)
VF (__v_tanf, -3.1, 3.1)
#ifdef __vpcs
VNF (__vn_asinhf, -10.0, 10.0)
VNF (_ZGVnN4v_asinhf, -10.0, 10.0)

VNF (__vn_atanf, -10.0, 10.0)
VNF (_ZGVnN4v_atanf, -10.0, 10.0)

VND (__vn_atan, -10.0, 10.0)
VND (_ZGVnN2v_atan, -10.0, 10.0)

{"__vn_atan2f", 'f', 'n', -10.0, 10.0, {.vnf = __vn_atan2f_wrap}},
{"_ZGVnN4vv_atan2f", 'f', 'n', -10.0, 10.0, {.vnf = _Z_atan2f_wrap}},

{"__vn_atan2", 'd', 'n', -10.0, 10.0, {.vnd = __vn_atan2_wrap}},
{"_ZGVnN2vv_atan2", 'd', 'n', -10.0, 10.0, {.vnd = _Z_atan2_wrap}},

VNF (__vn_coshf, -10.0, 10.0)
VNF (_ZGVnN4v_coshf, -10.0, 10.0)

VNF (__vn_erff, -4.0, 4.0)
VNF (_ZGVnN4v_erff, -4.0, 4.0)

VND (__vn_erf, -6.0, 6.0)
VND (_ZGVnN2v_erf, -6.0, 6.0)

VNF (__vn_erfcf, -6.0, 28.0)
VNF (_ZGVnN4v_erfcf, -6.0, 28.0)

VND (__vn_erfc, -6.0, 28.0)
VND (_ZGVnN2v_erfc, -6.0, 28.0)

VNF (__vn_expm1f, -9.9, 9.9)
VNF (_ZGVnN4v_expm1f, -9.9, 9.9)

VNF (__vn_log10f, 0.01, 11.1)
VNF (_ZGVnN4v_log10f, 0.01, 11.1)

VND (__vn_log10, 0.01, 11.1)
VND (_ZGVnN2v_log10, 0.01, 11.1)

VNF (__vn_log1pf, -0.9, 10.0)
VNF (_ZGVnN4v_log1pf, -0.9, 10.0)

VND (__vn_log1p, -0.9, 10.0)
VND (_ZGVnN2v_log1p, -0.9, 10.0)

VNF (__vn_log2f, 0.01, 11.1)
VNF (_ZGVnN4v_log2f, 0.01, 11.1)

VND (__vn_log2, 0.01, 11.1)
VND (_ZGVnN2v_log2, 0.01, 11.1)

VNF (__vn_sinhf, -10.0, 10.0)
VNF (_ZGVnN4v_sinhf, -10.0, 10.0)

VNF (__vn_tanf, -3.1, 3.1)
VNF (_ZGVnN4v_tanf, -3.1, 3.1)
#endif
#endif
#if WANT_SVE_MATH
SVF (__sv_atanf_x, -3.1, 3.1)
SVF (_ZGVsMxv_atanf, -3.1, 3.1)
SVD (__sv_atan_x, -3.1, 3.1)
SVD (_ZGVsMxv_atan, -3.1, 3.1)

{"__sv_atan2f_x", 'f', 'n', -10.0, 10.0, {.svf = __sv_atan2f_wrap}},
{"_ZGVsMxvv_atan2f", 'f', 'n', -10.0, 10.0, {.svf = _Z_sv_atan2f_wrap}},
{"__sv_atan2", 'd', 'n', -10.0, 10.0, {.svd = __sv_atan2_wrap}},
{"_ZGVsM2vv_atan2", 'd', 'n', -10.0, 10.0, {.svd = _Z_sv_atan2_wrap}},

SVF (__sv_erff_x, -4.0, 4.0)
SVF (_ZGVsMxv_erff, -4.0, 4.0)
SVD (__sv_erf_x, -4.0, 4.0)
SVD (_ZGVsMxv_erf, -4.0, 4.0)

SVD (__sv_erfc_x, -4, 10)
SVD (_ZGVsMxv_erfc, -4, 10)

SVF (__sv_expf_x, -9.9, 9.9)
SVF (_ZGVsMxv_expf, -9.9, 9.9)

SVF (__sv_cosf_x, -3.1, 3.1)
SVF (_ZGVsMxv_cosf, -3.1, 3.1)
SVF (__sv_sinf_x, -3.1, 3.1)
SVF (_ZGVsMxv_sinf, -3.1, 3.1)

SVF (__sv_logf_x, 0.01, 11.1)
SVF (_ZGVsMxv_logf, 0.01, 11.1)
SVD (__sv_log_x, 0.01, 11.1)
SVD (_ZGVsMxv_log, 0.01, 11.1)

SVF (__sv_log10f_x, 0.01, 11.1)
SVF (_ZGVsMxv_log10f, 0.01, 11.1)
SVD (__sv_log10_x, 0.01, 11.1)
SVD (_ZGVsMxv_log10, 0.01, 11.1)

SVD (__sv_cos_x, -3.1, 3.1)
SVD (_ZGVsMxv_cos, -3.1, 3.1)
SVD (__sv_sin_x, -3.1, 3.1)
SVD (_ZGVsMxv_sin, -3.1, 3.1)

SVF (__sv_tanf_x, -3.1, 3.1)
SVF (_ZGVsMxv_tanf, -3.1, 3.1)

{"__sv_powif_x", 'f', 'n', -10.0, 10.0, {.svf = __sv_powif_wrap}},
{"_ZGVsMxvv_powi", 'f', 'n', -10.0, 10.0, {.svf = _Z_sv_powi_wrap}},
{"__sv_powi_x", 'd', 'n', -10.0, 10.0, {.svd = __sv_powi_wrap}},
{"_ZGVsMxvv_powk", 'd', 'n', -10.0, 10.0, {.svd = _Z_sv_powk_wrap}},

#endif
#endif
  // clang-format on
