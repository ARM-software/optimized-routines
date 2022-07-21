// clang-format off
/*
 * Function wrappers for ulp.
 *
 * Copyright (c) 2022, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#if USE_MPFR
static int sincos_mpfr_sin(mpfr_t y, const mpfr_t x, mpfr_rnd_t r) {
  mpfr_cos(y, x, r);
  return mpfr_sin(y, x, r);
}
static int sincos_mpfr_cos(mpfr_t y, const mpfr_t x, mpfr_rnd_t r) {
  mpfr_sin(y, x, r);
  return mpfr_cos(y, x, r);
}
#endif

/* Wrappers for vector functions.  */
#if __aarch64__ && WANT_VMATH
static float v_asinhf(float x) { return __v_asinhf(argf(x))[0]; }
static float v_atanf(float x) { return __v_atanf(argf(x))[0]; }
static float v_atan2f(float x, float y) { return __v_atan2f(argf(x), argf(y))[0]; }
static float v_erff(float x) { return __v_erff(argf(x))[0]; }
static float v_erfcf(float x) { return __v_erfcf(argf(x))[0]; }
static float v_log10f(float x) { return __v_log10f(argf(x))[0]; }
static float v_log1pf(float x) { return __v_log1pf(argf(x))[0]; }
static double v_atan(double x) { return __v_atan(argd(x))[0]; }
static double v_atan2(double x, double y) { return __v_atan2(argd(x), argd(y))[0]; }
static double v_erf(double x) { return __v_erf(argd(x))[0]; }
static double v_erfc(double x) { return __v_erfc(argd(x))[0]; }
static double v_log10(double x) { return __v_log10(argd(x))[0]; }
#ifdef __vpcs
static float vn_asinhf(float x) { return __vn_asinhf(argf(x))[0]; }
static float vn_atanf(float x) { return __vn_atanf(argf(x))[0]; }
static float vn_atan2f(float x, float y) { return __vn_atan2f(argf(x), argf(y))[0]; }
static float vn_erff(float x) { return __vn_erff(argf(x))[0]; }
static float vn_erfcf(float x) { return __vn_erfcf(argf(x))[0]; }
static float vn_log10f(float x) { return __vn_log10f(argf(x))[0]; }
static float vn_log1pf(float x) { return __vn_log1pf(argf(x))[0]; }
static double vn_atan(double x) { return __vn_atan(argd(x))[0]; }
static double vn_atan2(double x, double y) { return __vn_atan2(argd(x), argd(y))[0]; }
static double vn_erf(double x) { return __vn_erf(argd(x))[0]; }
static double vn_erfc(double x) { return __vn_erfc(argd(x))[0]; }
static double vn_log10(double x) { return __vn_log10(argd(x))[0]; }

static float Z_asinhf(float x) { return _ZGVnN4v_asinhf(argf(x))[0]; }
static float Z_atanf(float x) { return _ZGVnN4v_atanf(argf(x))[0]; }
static float Z_atan2f(float x, float y) { return _ZGVnN4vv_atan2f(argf(x), argf(y))[0]; }
static float Z_erff(float x) { return _ZGVnN4v_erff(argf(x))[0]; }
static float Z_erfcf(float x) { return _ZGVnN4v_erfcf(argf(x))[0]; }
static float Z_log10f(float x) { return _ZGVnN4v_log10f(argf(x))[0]; }
static float Z_log1pf(float x) { return _ZGVnN4v_log1pf(argf(x))[0]; }
static double Z_atan(double x) { return _ZGVnN2v_atan(argd(x))[0]; }
static double Z_atan2(double x, double y) { return _ZGVnN2vv_atan2(argd(x), argd(y))[0]; }
static double Z_erf(double x) { return _ZGVnN2v_erf(argd(x))[0]; }
static double Z_erfc(double x) { return _ZGVnN2v_erfc(argd(x))[0]; }
static double Z_log10(double x) { return _ZGVnN2v_log10(argd(x))[0]; }
#endif
#if WANT_SVE_MATH
static float sv_cosf(float x) {
  return svretf(__sv_cosf_x(svargf(x), svptrue_b32()));
}
static float Z_sv_cosf(float x) {
  return svretf(_ZGVsMxv_cosf(svargf(x), svptrue_b32()));
}
#endif
#endif
// clang-format on
