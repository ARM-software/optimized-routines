/*
 * Function entries for mathbench.
 *
 * Copyright (c) 2022-2024, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */
/* clang-format off */
{"pow", 'd', 0, 0.01, 11.1, {.d = xypow}},
D (xpow, 0.01, 11.1)
D (ypow, -9.9, 9.9)
{"powf", 'f', 0, 0.01, 11.1, {.f = xypowf}},
F (xpowf, 0.01, 11.1)
F (ypowf, -9.9, 9.9)
{"sincosf", 'f', 0, 0.1, 0.7, {.f = sincosf_wrap}},
{"sincosf", 'f', 0, 0.8, 3.1, {.f = sincosf_wrap}},
{"sincosf", 'f', 0, -3.1, 3.1, {.f = sincosf_wrap}},
{"sincosf", 'f', 0, 3.3, 33.3, {.f = sincosf_wrap}},
{"sincosf", 'f', 0, 100, 1000, {.f = sincosf_wrap}},
{"sincosf", 'f', 0, 1e6, 1e32, {.f = sincosf_wrap}},
#if WANT_SIMD_TESTS && defined (__vpcs)
{"_ZGVnN2vv_pow", 'd', 'n', 0.01, 11.1, {.vnd = xy_Z_pow}},
VNF (_ZGVnN4v_expf_1u, -9.9, 9.9)
VNF (_ZGVnN4v_exp2f_1u, -9.9, 9.9)
{"_ZGVnN4vv_powf",  'f', 'n',   0.01, 11.1, {.vnf = xy_Z_powf}},
{"x_ZGVnN4vv_powf", 'f', 'n',   0.01, 11.1, {.vnf = x_Z_powf}},
{"y_ZGVnN4vv_powf", 'f', 'n', -10.0,  10.0, {.vnf = y_Z_powf}},
#endif
    /* clang-format on */

#define _ZSF1(fun, a, b) F (fun##f, a, b)
#define _ZSD1(f, a, b) D (f, a, b)

#define _ZVF1(fun, a, b) VNF (_ZGVnN4v_##fun##f, a, b)
#define _ZVD1(f, a, b) VND (_ZGVnN2v_##f, a, b)

#include "mathbench_funcs_gen.h"
