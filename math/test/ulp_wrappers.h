/*
 * Function wrappers for ulp.
 *
 * Copyright (c) 2022-2024, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

/* clang-format off */

/* Wrappers for sincos.  */
static float sincosf_sinf(float x) {(void)cosf(x); return sinf(x);}
static float sincosf_cosf(float x) {(void)sinf(x); return cosf(x);}
static double sincos_sin(double x) {(void)cos(x); return sin(x);}
static double sincos_cos(double x) {(void)sin(x); return cos(x);}
#if USE_MPFR
static int sincos_mpfr_sin(mpfr_t y, const mpfr_t x, mpfr_rnd_t r) { mpfr_cos(y,x,r); return mpfr_sin(y,x,r); }
static int sincos_mpfr_cos(mpfr_t y, const mpfr_t x, mpfr_rnd_t r) { mpfr_sin(y,x,r); return mpfr_cos(y,x,r); }
#endif

/* Wrappers for vector functions.  */
#if WANT_SIMD_TESTS
static float Z_expf_1u(float x) { return _ZGVnN4v_expf_1u(argf(x))[0]; }
static float Z_exp2f_1u(float x) { return _ZGVnN4v_exp2f_1u(argf(x))[0]; }
static float Z_powf(float x, float y) { return _ZGVnN4vv_powf(argf(x),argf(y))[0]; }
static double Z_pow(double x, double y) { return _ZGVnN2vv_pow(argd(x),argd(y))[0]; }
#endif

/* clang-format on */

/* No wrappers for scalar routines, but TEST_SIG will emit them.  */
#define ZSNF1_WRAP(func)
#define ZSNF2_WRAP(func)
#define ZSND1_WRAP(func)
#define ZSND2_WRAP(func)

#define ZVNF1_WRAP(func)                                                      \
  static float Z_##func##f (float x)                                          \
  {                                                                           \
    return _ZGVnN4v_##func##f (argf (x))[0];                                  \
  }
#define ZVNF2_WRAP(func)                                                      \
  static float Z_##func##f (float x, float y)                                 \
  {                                                                           \
    return _ZGVnN4vv_##func##f (argf (x), argf (y))[0];                       \
  }
#define ZVND1_WRAP(func)                                                      \
  static double Z_##func (double x) { return _ZGVnN2v_##func (argd (x))[0]; }
#define ZVND2_WRAP(func)                                                      \
  static double Z_##func (double x, double y)                                 \
  {                                                                           \
    return _ZGVnN2vv_##func (argd (x), argd (y))[0];                          \
  }

#if WANT_SVE_MATH
#  define ZSVNF1_WRAP(func)                                                   \
    static float Z_sv_##func##f (svbool_t pg, float x)                        \
    {                                                                         \
      return svretf (_ZGVsMxv_##func##f (svargf (x), pg), pg);                \
    }
#  define ZSVNF2_WRAP(func)                                                   \
    static float Z_sv_##func##f (svbool_t pg, float x, float y)               \
    {                                                                         \
      return svretf (_ZGVsMxvv_##func##f (svargf (x), svargf (y), pg), pg);   \
    }
#  define ZSVND1_WRAP(func)                                                   \
    static double Z_sv_##func (svbool_t pg, double x)                         \
    {                                                                         \
      return svretd (_ZGVsMxv_##func (svargd (x), pg), pg);                   \
    }
#  define ZSVND2_WRAP(func)                                                   \
    static double Z_sv_##func (svbool_t pg, double x, double y)               \
    {                                                                         \
      return svretd (_ZGVsMxvv_##func (svargd (x), svargd (y), pg), pg);      \
    }
#endif

#include "ulp_wrappers_gen.h"
