/*
 * Function entries for mathbench.
 *
 * Copyright (c) 2022, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */
F (erff, -4.0, 4.0)
F (log10f, 0.01, 11.1)

D (log10, 0.01, 11.1)

#if WANT_VMATH
F (__s_log10f, 0.01, 11.1)
D (__s_log10, 0.01, 11.1)
#if __aarch64__
VD (__v_log10, 0.01, 11.1)
VF (__v_log10f, 0.01, 11.1)
#ifdef __vpcs
VNF (__vn_log10f, 0.01, 11.1)
VNF (_ZGVnN4v_log10f, 0.01, 11.1)

VND (__vn_log10, 0.01, 11.1)
VND (_ZGVnN2v_log10, 0.01, 11.1)
#endif
#endif
#endif
