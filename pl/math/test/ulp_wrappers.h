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

#if USE_MPFR
static int wrap_mpfr_powi(mpfr_t ret, const mpfr_t x, const mpfr_t y, mpfr_rnd_t rnd) {
  mpfr_t y2;
  mpfr_init(y2);
  mpfr_trunc(y2, y);
  return mpfr_pow(ret, x, y2, rnd);
}
# if MPFR_VERSION < MPFR_VERSION_NUM(4, 2, 0)
int mpfr_tanpi(mpfr_t ret, const mpfr_t arg, mpfr_rnd_t rnd);
int mpfr_sinpi(mpfr_t ret, const mpfr_t arg, mpfr_rnd_t rnd);
int mpfr_cospi(mpfr_t ret, const mpfr_t arg, mpfr_rnd_t rnd);
# endif
#endif

/* Our implementations of powi/powk are too imprecise to verify
   against any established pow implementation. Instead we have the
   following simple implementation, against which it is enough to
   maintain bitwise reproducibility. Note the test framework expects
   the reference impl to be of higher precision than the function
   under test. For instance this means that the reference for
   double-precision powi will be passed a long double, so to check
   bitwise reproducibility we have to cast it back down to
   double. This is fine since a round-trip to higher precision and
   back down is correctly rounded.  */
#define DECL_POW_INT_REF(NAME, DBL_T, FLT_T, INT_T)                            \
  static DBL_T __attribute__((unused)) NAME (DBL_T in_val, DBL_T y)            \
  {                                                                            \
    INT_T n = (INT_T) round (y);                                               \
    FLT_T acc = 1.0;                                                           \
    bool want_recip = n < 0;                                                   \
    n = n < 0 ? -n : n;                                                        \
                                                                               \
    for (FLT_T c = in_val; n; c *= c, n >>= 1)                                 \
      {                                                                        \
        if (n & 0x1)                                                           \
          {                                                                    \
            acc *= c;                                                          \
          }                                                                    \
      }                                                                        \
    if (want_recip)                                                            \
      {                                                                        \
        acc = 1.0 / acc;                                                       \
      }                                                                        \
    return acc;                                                                \
  }

DECL_POW_INT_REF(ref_powif, double, float, int)
DECL_POW_INT_REF(ref_powi, long double, double, int)

#define ZVF1_WRAP(func) static float Z_##func##f(float x) { return _ZGVnN4v_##func##f(argf(x))[0]; }
#define ZVF2_WRAP(func) static float Z_##func##f(float x, float y) { return _ZGVnN4vv_##func##f(argf(x), argf(y))[0]; }
#define ZVD1_WRAP(func) static double Z_##func(double x) { return _ZGVnN2v_##func(argd(x))[0]; }
#define ZVD2_WRAP(func) static double Z_##func(double x, double y) { return _ZGVnN2vv_##func(argd(x), argd(y))[0]; }

#if defined(__vpcs) && __aarch64__

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

/* No wrappers for scalar routines, but PL_SIG will emit them.  */
#define ZSNF1_WRAP(func)
#define ZSNF2_WRAP(func)
#define ZSND1_WRAP(func)
#define ZSND2_WRAP(func)

#include "ulp_wrappers_gen.h"

#if USE_MPFR
static int modf_mpfr_frac(mpfr_t f, const mpfr_t x, mpfr_rnd_t r) { MPFR_DECL_INIT(i, 80); return mpfr_modf(i,f,x,r); }
static int modf_mpfr_int(mpfr_t i, const mpfr_t x, mpfr_rnd_t r) { MPFR_DECL_INIT(f, 80); return mpfr_modf(i,f,x,r); }
#endif

float modff_frac(float x) { float i; return modff(x, &i); }
float modff_int(float x) { float i; modff(x, &i); return i; }
double modf_frac(double x) { double i; return modf(x, &i); }
double modf_int(double x) { double i; modf(x, &i); return i; }
long double modfl_frac(long double x) { long double i; return modfl(x, &i); }
long double modfl_int(long double x) { long double i; modfl(x, &i); return i; }

float sincospif_sin(float x) { float s, c; sincospif(x, &s, &c); return s; }
float sincospif_cos(float x) { float s, c; sincospif(x, &s, &c); return c; }
double sincospi_sin(double x) { double s, c; sincospi(x, &s, &c); return s; }
double sincospi_cos(double x) { double s, c; sincospi(x, &s, &c); return c; }

#if __linux__

float v_sincosf_sin(float x) { float32x4_t s, c; _ZGVnN4vl4l4_sincosf(vdupq_n_f32(x), &s, &c); return s[0]; }
float v_sincosf_cos(float x) { float32x4_t s, c; _ZGVnN4vl4l4_sincosf(vdupq_n_f32(x), &s, &c); return c[0]; }
float v_cexpif_sin(float x) { return _ZGVnN4v_cexpif(vdupq_n_f32(x)).val[0][0]; }
float v_cexpif_cos(float x) { return _ZGVnN4v_cexpif(vdupq_n_f32(x)).val[1][0]; }
float v_modff_frac(float x) { float32x4_t y; return _ZGVnN4vl4_modff(vdupq_n_f32(x), &y)[0]; }
float v_modff_int(float x) { float32x4_t y; _ZGVnN4vl4_modff(vdupq_n_f32(x), &y); return y[0]; }

double v_sincos_sin(double x) { float64x2_t s, c; _ZGVnN2vl8l8_sincos(vdupq_n_f64(x), &s, &c); return s[0]; }
double v_sincos_cos(double x) { float64x2_t s, c; _ZGVnN2vl8l8_sincos(vdupq_n_f64(x), &s, &c); return c[0]; }
double v_sincospi_sin(double x) { float64x2_t s, c; _ZGVnN2vl8l8_sincospi(vdupq_n_f64(x), &s, &c); return s[0]; }
double v_sincospi_cos(double x) { float64x2_t s, c; _ZGVnN2vl8l8_sincospi(vdupq_n_f64(x), &s, &c); return c[0]; }
double v_cexpi_sin(double x) { return _ZGVnN2v_cexpi(vdupq_n_f64(x)).val[0][0]; }
double v_cexpi_cos(double x) { return _ZGVnN2v_cexpi(vdupq_n_f64(x)).val[1][0]; }
double v_modf_frac(double x) { float64x2_t y; return _ZGVnN2vl8_modf(vdupq_n_f64(x), &y)[0]; }
double v_modf_int(double x) { float64x2_t y; _ZGVnN2vl8_modf(vdupq_n_f64(x), &y); return y[0]; }

float v_sincospif_sin(float x) { float32x4_t s, c; _ZGVnN4vl4l4_sincospif(vdupq_n_f32(x), &s, &c); return s[0]; }
float v_sincospif_cos(float x) { float32x4_t s, c; _ZGVnN4vl4l4_sincospif(vdupq_n_f32(x), &s, &c); return c[0]; }

# if WANT_SVE_MATH
static float Z_sv_powi(svbool_t pg, float x, float y) { return svretf(_ZGVsMxvv_powi(svargf(x), svdup_s32((int)round(y)), pg), pg); }
static double Z_sv_powk(svbool_t pg, double x, double y) { return svretd(_ZGVsMxvv_powk(svargd(x), svdup_s64((long)round(y)), pg), pg); }

float sv_sincosf_sin(svbool_t pg, float x) { float s[svcntw()], c[svcntw()]; _ZGVsMxvl4l4_sincosf(svdup_f32(x), s, c, pg); return svretf(svld1(pg, s), pg); }
float sv_sincosf_cos(svbool_t pg, float x) { float s[svcntw()], c[svcntw()]; _ZGVsMxvl4l4_sincosf(svdup_f32(x), s, c, pg); return svretf(svld1(pg, c), pg); }
float sv_sincospif_sin(svbool_t pg, float x) { float s[svcntw()], c[svcntw()]; _ZGVsMxvl4l4_sincospif(svdup_f32(x), s, c, pg); return svretf(svld1(pg, s), pg); }
float sv_sincospif_cos(svbool_t pg, float x) { float s[svcntw()], c[svcntw()]; _ZGVsMxvl4l4_sincospif(svdup_f32(x), s, c, pg); return svretf(svld1(pg, c), pg); }
float sv_cexpif_sin(svbool_t pg, float x) { return svretf(svget2(_ZGVsMxv_cexpif(svdup_f32(x), pg), 0), pg); }
float sv_cexpif_cos(svbool_t pg, float x) { return svretf(svget2(_ZGVsMxv_cexpif(svdup_f32(x), pg), 1), pg); }
float sv_modff_frac(svbool_t pg, float x) { float i[svcntw()]; return svretf(_ZGVsMxvl4_modff(svdup_f32(x), i, pg), pg); }
float sv_modff_int(svbool_t pg, float x) { float i[svcntw()]; _ZGVsMxvl4_modff(svdup_f32(x), i, pg); return svretf(svld1(pg, i), pg); }

double sv_sincos_sin(svbool_t pg, double x) { double s[svcntd()], c[svcntd()]; _ZGVsMxvl8l8_sincos(svdup_f64(x), s, c, pg); return svretd(svld1(pg, s), pg); }
double sv_sincos_cos(svbool_t pg, double x) { double s[svcntd()], c[svcntd()]; _ZGVsMxvl8l8_sincos(svdup_f64(x), s, c, pg); return svretd(svld1(pg, c), pg); }
double sv_sincospi_sin(svbool_t pg, double x) { double s[svcntd()], c[svcntd()]; _ZGVsMxvl8l8_sincospi(svdup_f64(x), s, c, pg); return svretd(svld1(pg, s), pg); }
double sv_sincospi_cos(svbool_t pg, double x) { double s[svcntd()], c[svcntd()]; _ZGVsMxvl8l8_sincospi(svdup_f64(x), s, c, pg); return svretd(svld1(pg, c), pg); }
double sv_cexpi_sin(svbool_t pg, double x) { return svretd(svget2(_ZGVsMxv_cexpi(svdup_f64(x), pg), 0), pg); }
double sv_cexpi_cos(svbool_t pg, double x) { return svretd(svget2(_ZGVsMxv_cexpi(svdup_f64(x), pg), 1), pg); }
double sv_modf_frac(svbool_t pg, double x) { double i[svcntd()]; return svretd(_ZGVsMxvl8_modf(svdup_f64(x), i, pg), pg); }
double sv_modf_int(svbool_t pg, double x) { double i[svcntd()]; _ZGVsMxvl8_modf(svdup_f64(x), i, pg); return svretd(svld1(pg, i), pg); }
# endif
#endif
// clang-format on
