/*
 * Function wrappers for ulp.
 *
 * Copyright (c) 2022-2024, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

/* clang-format off */

#if WANT_SIMD_TESTS
#include <arm_neon.h>
#endif

/* Wrappers for sincos.  */
static float sincosf_sinf(float x) {(void)cosf(x); return sinf(x);}
static float sincosf_cosf(float x) {(void)sinf(x); return cosf(x);}
static double sincos_sin(double x) {(void)cos(x); return sin(x);}
static double sincos_cos(double x) {(void)sin(x); return cos(x);}
#if USE_MPFR
static int sincos_mpfr_sin(mpfr_t y, const mpfr_t x, mpfr_rnd_t r) { mpfr_cos(y,x,r); return mpfr_sin(y,x,r); }
static int sincos_mpfr_cos(mpfr_t y, const mpfr_t x, mpfr_rnd_t r) { mpfr_sin(y,x,r); return mpfr_cos(y,x,r); }
# if MPFR_VERSION < MPFR_VERSION_NUM(4, 2, 0)
static int mpfr_sinpi (mpfr_t ret, const mpfr_t arg, mpfr_rnd_t rnd) {
  MPFR_DECL_INIT (frd, 1080);
  mpfr_const_pi (frd, GMP_RNDN);
  mpfr_mul (frd, frd, arg, GMP_RNDN);
  return mpfr_sin (ret, frd, GMP_RNDN);
}

static int mpfr_cospi (mpfr_t ret, const mpfr_t arg, mpfr_rnd_t rnd) {
  MPFR_DECL_INIT (frd, 1080);
  mpfr_const_pi (frd, GMP_RNDN);
  mpfr_mul (frd, frd, arg, GMP_RNDN);
  return mpfr_cos (ret, frd, GMP_RNDN);
}
# endif

#endif

/* Wrappers for vector functions.  */
#if WANT_SIMD_TESTS
static float Z_expf_1u(float x) { return _ZGVnN4v_expf_1u(argf(x))[0]; }
static float Z_exp2f_1u(float x) { return _ZGVnN4v_exp2f_1u(argf(x))[0]; }
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

#if WANT_TRIGPI_TESTS
float
arm_math_sincospif_sin (float x)
{
  float s, c;
  arm_math_sincospif (x, &s, &c);
  return s;
}
float
arm_math_sincospif_cos (float x)
{
  float s, c;
  arm_math_sincospif (x, &s, &c);
  return c;
}
double
arm_math_sincospi_sin (double x)
{
  double s, c;
  arm_math_sincospi (x, &s, &c);
  return s;
}
double
arm_math_sincospi_cos (double x)
{
  double s, c;
  arm_math_sincospi (x, &s, &c);
  return c;
}

# if WANT_SIMD_TESTS
ZVNF1_WRAP (cospi)
ZVND1_WRAP (cospi)
ZVNF1_WRAP (sinpi)
ZVND1_WRAP (sinpi)

double
v_sincospi_sin (double x)
{
  float64x2_t s, c;
  _ZGVnN2vl8l8_sincospi (vdupq_n_f64 (x), &s, &c);
  return s[0];
}
double
v_sincospi_cos (double x)
{
  float64x2_t s, c;
  _ZGVnN2vl8l8_sincospi (vdupq_n_f64 (x), &s, &c);
  return c[0];
}
float
v_sincospif_sin (float x)
{
  float32x4_t s, c;
  _ZGVnN4vl4l4_sincospif (vdupq_n_f32 (x), &s, &c);
  return s[0];
}
float
v_sincospif_cos (float x)
{
  float32x4_t s, c;
  _ZGVnN4vl4l4_sincospif (vdupq_n_f32 (x), &s, &c);
  return c[0];
}
# endif
#endif

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

#  if WANT_TRIGPI_TESTS
ZSVNF1_WRAP (cospi)
ZSVND1_WRAP (cospi)
ZSVNF1_WRAP (sinpi)
ZSVND1_WRAP (sinpi)
double
sv_sincospi_sin (svbool_t pg, double x)
{
  double s[svcntd ()], c[svcntd ()];
  _ZGVsMxvl8l8_sincospi (svdup_f64 (x), s, c, pg);
  return svretd (svld1 (pg, s), pg);
}
double
sv_sincospi_cos (svbool_t pg, double x)
{
  double s[svcntd ()], c[svcntd ()];
  _ZGVsMxvl8l8_sincospi (svdup_f64 (x), s, c, pg);
  return svretd (svld1 (pg, c), pg);
}
float
sv_sincospif_sin (svbool_t pg, float x)
{
  float s[svcntw ()], c[svcntw ()];
  _ZGVsMxvl4l4_sincospif (svdup_f32 (x), s, c, pg);
  return svretf (svld1 (pg, s), pg);
}
float
sv_sincospif_cos (svbool_t pg, float x)
{
  float s[svcntw ()], c[svcntw ()];
  _ZGVsMxvl4l4_sincospif (svdup_f32 (x), s, c, pg);
  return svretf (svld1 (pg, c), pg);
}
#  endif
#endif

#include "ulp_wrappers_gen.h"
