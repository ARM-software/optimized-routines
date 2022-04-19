/*
 * Function wrappers for ulp.
 *
 * Copyright (c) 2022, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#if USE_MPFR
static int sincos_mpfr_sin(mpfr_t y, const mpfr_t x, mpfr_rnd_t r) { mpfr_cos(y,x,r); return mpfr_sin(y,x,r); }
static int sincos_mpfr_cos(mpfr_t y, const mpfr_t x, mpfr_rnd_t r) { mpfr_sin(y,x,r); return mpfr_cos(y,x,r); }
#endif

/* A bit of a hack: call vector functions twice with the same
   input in lane 0 but a different value in other lanes: once
   with an in-range value and then with a special case value.  */
static int secondcall;

/* Wrappers for vector functions.  */
#if __aarch64__ && WANT_VMATH
typedef __f32x4_t v_float;
typedef __f64x2_t v_double;
static const float fv[2] = {1.0f, -INFINITY};
static const double dv[2] = {1.0, -INFINITY};
static inline v_float argf(float x) { return (v_float){x,x,x,fv[secondcall]}; }
static inline v_double argd(double x) { return (v_double){x,dv[secondcall]}; }

static float v_log10f(float x) { return __v_log10f(argf(x))[0]; }
#ifdef __vpcs
static float vn_log10f(float x) { return __vn_log10f(argf(x))[0]; }
static float Z_log10f(float x) { return _ZGVnN4v_log10f(argf(x))[0]; }
#endif
#endif
