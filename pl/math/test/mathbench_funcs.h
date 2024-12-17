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

    // clang-format on
