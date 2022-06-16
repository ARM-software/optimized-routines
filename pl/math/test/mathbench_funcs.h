// clang-format off
/*
 * Function entries for mathbench.
 *
 * Copyright (c) 2022, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */
{"atan2f", 'f', 0, -10.0, 10.0, {.f = atan2f_wrap}},
F (erfcf, -4.0, 10.0)
F (erff, -4.0, 4.0)
F (log10f, 0.01, 11.1)

{"atan2", 'd', 0, -10.0, 10.0, {.d = atan2_wrap}},
D (erf, -6,6)
D (erfc, -6.0, 28.0)
D (log10, 0.01, 11.1)

#if WANT_VMATH
{"__s_atan2", 'd', 0, -10.0, 10.0, {.d = __s_atan2_wrap}},
F (__s_erff, -4.0, 4.0)
D (__s_erf, -6.0, 6.0)
F (__s_erfcf, -6.0, 28.0)
D (__s_erfc, -6.0, 28.0)
F (__s_log10f, 0.01, 11.1)
D (__s_log10, 0.01, 11.1)
#if __aarch64__
{"__v_atan2", 'd', 'v', -10.0, 10.0, {.vd = __v_atan2_wrap}},
VF  (__v_erff, -4.0, 4.0)
VD  (__v_erf, -6.0, 6.0)
VF (__v_erfcf, -6.0, 28.0)
VD (__v_erfc, -6.0, 28.0)
VD (__v_log10, 0.01, 11.1)
VF (__v_log10f, 0.01, 11.1)
#ifdef __vpcs
{"__vn_atan2", 'd', 'n', -10.0, 10.0, {.vnd = __vn_atan2_wrap}},
{"_ZGVnN2vv_atan2", 'd', 'n', -10.0, 10.0, {.vnd = _Z_atan2_wrap}},

VNF (__vn_erff, -4.0, 4.0)
VNF (_ZGVnN4v_erff, -4.0, 4.0)

VND (__vn_erf, -6.0, 6.0)
VND (_ZGVnN2v_erf, -6.0, 6.0)

VNF (__vn_erfcf, -6.0, 28.0)
VNF (_ZGVnN4v_erfcf, -6.0, 28.0)

VND (__vn_erfc, -6.0, 28.0)
VND (_ZGVnN2v_erfc, -6.0, 28.0)

VNF (__vn_log10f, 0.01, 11.1)
VNF (_ZGVnN4v_log10f, 0.01, 11.1)

VND (__vn_log10, 0.01, 11.1)
VND (_ZGVnN2v_log10, 0.01, 11.1)
#endif
#endif
#endif
  // clang-format on
