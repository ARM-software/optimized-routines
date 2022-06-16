/*
 * Function entries for ulp.
 *
 * Copyright (c) 2022, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */
F2 (atan2)
F1 (erfc)
F1 (erf)
F1 (log10)
D2 (atan2)
D1 (erfc)
D1 (log10)
#if WANT_VMATH
F (__s_atan, __s_atan, atanl, mpfr_atan, 1, 0, d1, 0)
F (__s_atan2, __s_atan2, atan2l, mpfr_atan2, 2, 0, d2, 0)
F (__s_erff, __s_erff, erf, mpfr_erf, 1, 1, f1, 0)
F (__s_erf, __s_erf, erfl, mpfr_erf, 1, 0, d1, 0)
F (__s_erfcf, __s_erfcf, erfc, mpfr_erfc, 1, 1, f1, 0)
F (__s_erfc, __s_erfc, erfcl, mpfr_erfc, 1, 0, d1, 0)
F (__s_log10f, __s_log10f, log10, mpfr_log10, 1, 1, f1, 0)
F (__s_log10, __s_log10, log10l, mpfr_log10, 1, 0, d1, 0)
#if __aarch64__
F (__v_atan, v_atan, atanl, mpfr_atan, 1, 0, d1, 1)
F (__v_atan2, v_atan2, atan2l, mpfr_atan2, 2, 0, d2, 1)
F (__v_erff, v_erff, erf, mpfr_erf, 1, 1, f1, 1)
F (__v_erf, v_erf, erfl, mpfr_erf, 1, 0, d1, 1)
F (__v_erfcf, v_erfcf, erfc, mpfr_erfc, 1, 1, f1, 1)
F (__v_erfc, v_erfc, erfcl, mpfr_erfc, 1, 0, d1, 1)
F (__v_log10f, v_log10f, log10, mpfr_log10, 1, 1, f1, 1)
F (__v_log10, v_log10, log10l, mpfr_log10, 1, 0, d1, 1)
#ifdef __vpcs
F (__vn_atan, vn_atan, atanl, mpfr_atan, 1, 0, d1, 1)
F (__vn_atan2, vn_atan2, atan2l, mpfr_atan2, 2, 0, d2, 1)
F (__vn_erff, vn_erff, erf, mpfr_erf, 1, 1, f1, 1)
F (__vn_erf, vn_erf, erfl, mpfr_erf, 1, 0, d1, 1)
F (__vn_erfcf, vn_erfcf, erfc, mpfr_erfc, 1, 1, f1, 1)
F (__vn_erfc, vn_erfc, erfcl, mpfr_erfc, 1, 0, d1, 1)
F (__vn_log10f, vn_log10f, log10, mpfr_log10, 1, 1, f1, 1)
F (__vn_log10, vn_log10, log10l, mpfr_log10, 1, 0, d1, 1)
F (_ZGVnN2v_atan, Z_atan, atanl, mpfr_atan, 1, 0, d1, 1)
F (_ZGVnN2vv_atan2, Z_atan2, atan2l, mpfr_atan2, 2, 0, d2, 1)
F (_ZGVnN4v_erff, Z_erff, erf, mpfr_erf, 1, 1, f1, 1)
F (_ZGVnN2v_erf, Z_erf, erfl, mpfr_erf, 1, 0, d1, 1)
F (_ZGVnN4v_erfcf, Z_erfcf, erfc, mpfr_erfc, 1, 1, f1, 1)
F (_ZGVnN2v_erfc, Z_erfc, erfcl, mpfr_erfc, 1, 0, d1, 1)
F (_ZGVnN4v_log10f, Z_log10f, log10, mpfr_log10, 1, 1, f1, 1)
F (_ZGVnN2v_log10, Z_log10, log10l, mpfr_log10, 1, 0, d1, 1)
#endif
#endif
#endif
