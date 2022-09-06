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

#define VF1_WRAP(func) static float v_##func##f(float x) { return __v_##func##f(argf(x))[0]; }
#define VF2_WRAP(func) static float v_##func##f(float x, float y) { return __v_##func##f(argf(x), argf(y))[0]; }
#define VD1_WRAP(func) static double v_##func(double x) { return __v_##func(argd(x))[0]; }
#define VD2_WRAP(func) static double v_##func(double x, double y) { return __v_##func(argd(x), argd(y))[0]; }

#define VNF1_WRAP(func) static float vn_##func##f(float x) { return __vn_##func##f(argf(x))[0]; }
#define VNF2_WRAP(func) static float vn_##func##f(float x, float y) { return __vn_##func##f(argf(x), argf(y))[0]; }
#define VND1_WRAP(func) static double vn_##func(double x) { return __vn_##func(argd(x))[0]; }
#define VND2_WRAP(func) static double vn_##func(double x, double y) { return __vn_##func(argd(x), argd(y))[0]; }

#define ZVF1_WRAP(func) static float Z_##func##f(float x) { return _ZGVnN4v_##func##f(argf(x))[0]; }
#define ZVF2_WRAP(func) static float Z_##func##f(float x, float y) { return _ZGVnN4vv_##func##f(argf(x), argf(y))[0]; }
#define ZVD1_WRAP(func) static double Z_##func(double x) { return _ZGVnN2v_##func(argd(x))[0]; }
#define ZVD2_WRAP(func) static double Z_##func(double x, double y) { return _ZGVnN2vv_##func(argd(x), argd(y))[0]; }

#define ZVNF1_WRAP(func) VNF1_WRAP(func) ZVF1_WRAP(func)
#define ZVNF2_WRAP(func) VNF2_WRAP(func) ZVF2_WRAP(func)
#define ZVND1_WRAP(func) VND1_WRAP(func) ZVD1_WRAP(func)
#define ZVND2_WRAP(func) VND2_WRAP(func) ZVD2_WRAP(func)

#define SVF1_WRAP(func) static float sv_##func##f(float x) { return svretf(__sv_##func##f_x(svargf(x), svptrue_b32())); }
#define SVF2_WRAP(func) static float sv_##func##f(float x, float y) { return svretf(__sv_##func##f_x(svargf(x), svargf(y), svptrue_b32())); }
#define SVD1_WRAP(func) static double sv_##func(double x) { return svretd(__sv_##func##_x(svargd(x), svptrue_b64())); }
#define SVD2_WRAP(func) static double sv_##func(double x, double y) { return svretd(__sv_##func##_x(svargd(x), svargd(y), svptrue_b64())); }

#define ZSVF1_WRAP(func) static float Z_sv_##func##f(float x) { return svretf(_ZGVsMxv_##func##f(svargf(x), svptrue_b32())); }
#define ZSVF2_WRAP(func) static float Z_sv_##func##f(float x, float y) { return svretf(_ZGVsMxvv_##func##f(svargf(x), svargf(y), svptrue_b32())); }
#define ZSVD1_WRAP(func) static double Z_sv_##func(double x) { return svretd(_ZGVsMxv_##func(svargd(x), svptrue_b64())); }
#define ZSVD2_WRAP(func) static double Z_sv_##func(double x, double y) { return svretd(_ZGVsMxvv_##func(svargd(x), svargd(y), svptrue_b64())); }

#define ZSVNF1_WRAP(func) SVF1_WRAP(func) ZSVF1_WRAP(func)
#define ZSVNF2_WRAP(func) SVF2_WRAP(func) ZSVF2_WRAP(func)
#define ZSVND1_WRAP(func) SVD1_WRAP(func) ZSVD1_WRAP(func)
#define ZSVND2_WRAP(func) SVD2_WRAP(func) ZSVD2_WRAP(func)

/* Wrappers for vector functions.  */
#if __aarch64__ && WANT_VMATH
VF1_WRAP(asinh)
VF1_WRAP(atan)
VF2_WRAP(atan2)
VF1_WRAP(erf)
VF1_WRAP(erfc)
VF1_WRAP(log10)
VF1_WRAP(log1p)
VF1_WRAP(log2)
VD1_WRAP(atan)
VD2_WRAP(atan2)
VD1_WRAP(erf)
VD1_WRAP(erfc)
VD1_WRAP(log10)
VD1_WRAP(log2)
#ifdef __vpcs
ZVNF1_WRAP(asinh)
ZVNF1_WRAP(atan)
ZVNF2_WRAP(atan2)
ZVNF1_WRAP(erf)
ZVNF1_WRAP(erfc)
ZVNF1_WRAP(log10)
ZVNF1_WRAP(log1p)
ZVNF1_WRAP(log2)
ZVND1_WRAP(atan)
ZVND2_WRAP(atan2)
ZVND1_WRAP(erf)
ZVND1_WRAP(erfc)
ZVND1_WRAP(log10)
ZVND1_WRAP(log2)
#endif
#if WANT_SVE_MATH
ZSVNF2_WRAP(atan2)
ZSVNF1_WRAP(atan)
ZSVNF1_WRAP(cos)
ZSVNF1_WRAP(log10)
ZSVNF1_WRAP(sin)

ZSVND2_WRAP(atan2)
ZSVND1_WRAP(atan)
ZSVND1_WRAP(cos)
ZSVND1_WRAP(log10)
ZSVND1_WRAP(sin)
#endif
#endif
// clang-format on
