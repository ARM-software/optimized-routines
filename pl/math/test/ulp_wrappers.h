// clang-format off
/*
 * Function wrappers for ulp.
 *
 * Copyright (c) 2022-2024, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#define _GNU_SOURCE
#include <stdbool.h>
#include <arm_neon.h>

#define ZVF1_WRAP(func) static float Z_##func##f(float x) { return _ZGVnN4v_##func##f(argf(x))[0]; }
#define ZVF2_WRAP(func) static float Z_##func##f(float x, float y) { return _ZGVnN4vv_##func##f(argf(x), argf(y))[0]; }
#define ZVD1_WRAP(func) static double Z_##func(double x) { return _ZGVnN2v_##func(argd(x))[0]; }
#define ZVD2_WRAP(func) static double Z_##func(double x, double y) { return _ZGVnN2vv_##func(argd(x), argd(y))[0]; }

#if WANT_SIMD_TESTS

#define ZVNF1_WRAP(func) ZVF1_WRAP(func)
#define ZVNF2_WRAP(func) ZVF2_WRAP(func)
#define ZVND1_WRAP(func) ZVD1_WRAP(func)
#define ZVND2_WRAP(func) ZVD2_WRAP(func)

#else

#define ZVNF1_WRAP(func)
#define ZVNF2_WRAP(func)
#define ZVND1_WRAP(func)
#define ZVND2_WRAP(func)

#endif

#define ZSVF1_WRAP(func) static float Z_sv_##func##f(svbool_t pg, float x) { return svretf(_ZGVsMxv_##func##f(svargf(x), pg), pg); }
#define ZSVF2_WRAP(func) static float Z_sv_##func##f(svbool_t pg, float x, float y) { return svretf(_ZGVsMxvv_##func##f(svargf(x), svargf(y), pg), pg); }
#define ZSVD1_WRAP(func) static double Z_sv_##func(svbool_t pg, double x) { return svretd(_ZGVsMxv_##func(svargd(x), pg), pg); }
#define ZSVD2_WRAP(func) static double Z_sv_##func(svbool_t pg, double x, double y) { return svretd(_ZGVsMxvv_##func(svargd(x), svargd(y), pg), pg); }

#if WANT_SVE_MATH

#define ZSVNF1_WRAP(func) ZSVF1_WRAP(func)
#define ZSVNF2_WRAP(func) ZSVF2_WRAP(func)
#define ZSVND1_WRAP(func) ZSVD1_WRAP(func)
#define ZSVND2_WRAP(func) ZSVD2_WRAP(func)

#else

#define ZSVNF1_WRAP(func)
#define ZSVNF2_WRAP(func)
#define ZSVND1_WRAP(func)
#define ZSVND2_WRAP(func)

#endif

/* No wrappers for scalar routines, but TEST_SIG will emit them.  */
#define ZSNF1_WRAP(func)
#define ZSNF2_WRAP(func)
#define ZSND1_WRAP(func)
#define ZSND2_WRAP(func)

#include "ulp_wrappers_gen.h"

#if __linux__ &&  WANT_SVE_MATH
#endif
// clang-format on
