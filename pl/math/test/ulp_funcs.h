/*
 * Function entries for ulp.
 *
 * Copyright (c) 2022, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */
F1 (erf)
F1 (log10)
D1 (log10)
#if WANT_VMATH
F (__s_log10f, __s_log10f, log10, mpfr_log10, 1, 1, f1, 0)
#if __aarch64__
F (__v_log10f, v_log10f, log10, mpfr_log10, 1, 1, f1, 1)
#ifdef __vpcs
F (__vn_log10f, vn_log10f, log10, mpfr_log10, 1, 1, f1, 1)
F (_ZGVnN4v_log10f, Z_log10f, log10, mpfr_log10, 1, 1, f1, 1)
#endif
#endif
#endif
