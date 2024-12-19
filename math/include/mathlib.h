/*
 * Public API.
 *
 * Copyright (c) 2015-2024, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#ifndef _MATHLIB_H
#define _MATHLIB_H

float expf (float);
float exp2f (float);
float logf (float);
float log10f (float);
float log2f (float);
float powf (float, float);
float sinf (float);
float cosf (float);
void sincosf (float, float*, float*);

double exp (double);
double exp10 (double);
double exp2 (double);
double log (double);
double log2 (double);
double pow (double, double);

#if __aarch64__
/* Low-accuracy scalar implementations of C23 routines.  */
float arm_math_cospif (float);
double arm_math_cospi (double);
float arm_math_sinpif (float);
double arm_math_sinpi (double);
float arm_math_tanpif (float);
double arm_math_tanpi (double);
void arm_math_sincospif (float, float *, float *);
void arm_math_sincospi (double, double *, double *);
#endif

#if WANT_EXPERIMENTAL_MATH

float acosf (float);
float acoshf (float);
float arm_math_erff (float);
float asinf (float);
float asinhf (float);
float atan2f (float, float);
float atanf (float);
float atanhf (float);
float cbrtf (float);
float coshf (float);
float cospif (float);
float erfcf (float);
float erfcf (float);
float erfinvf (float);
float expm1f (float);
float log10f (float);
float log1pf (float);
float sinhf (float);
float sinpif (float);
float tanf (float);
float tanhf (float);
float tanpif (float);

double acos (double);
double acosh (double);
double arm_math_erf (double);
double asin (double);
double asinh (double);
double atan (double);
double atan2 (double, double);
double atanh (double);
double cbrt (double);
double cosh (double);
double cospi (double);
double erfc (double);
double erfinv (double);
double expm1 (double);
double log10 (double);
double log1p (double);
double sinh (double);
double sinpi (double);
double tanh (double);
double tanpi (double);

long double erfinvl (long double);

#endif

#if __aarch64__ && __linux__
# include <arm_neon.h>
# undef __vpcs
# define __vpcs __attribute__((__aarch64_vector_pcs__))

/* Vector functions following the vector PCS using ABI names.  */
__vpcs float32x4_t _ZGVnN4v_acosf (float32x4_t);
__vpcs float32x4_t _ZGVnN4v_acoshf (float32x4_t);
__vpcs float32x4_t _ZGVnN4v_asinf (float32x4_t);
__vpcs float32x4_t _ZGVnN4v_asinhf (float32x4_t);
__vpcs float32x4_t _ZGVnN4v_atanf (float32x4_t);
__vpcs float32x4_t _ZGVnN4v_atanhf (float32x4_t);
__vpcs float32x4_t _ZGVnN4v_cbrtf (float32x4_t);
__vpcs float32x4_t _ZGVnN4v_cosf (float32x4_t);
__vpcs float32x4_t _ZGVnN4v_coshf (float32x4_t);
__vpcs float32x4_t _ZGVnN4v_cospif (float32x4_t);
__vpcs float32x4_t _ZGVnN4v_erfcf (float32x4_t);
__vpcs float32x4_t _ZGVnN4v_erff (float32x4_t);
__vpcs float32x4_t _ZGVnN4v_exp10f (float32x4_t);
__vpcs float32x4_t _ZGVnN4v_exp2f (float32x4_t);
__vpcs float32x4_t _ZGVnN4v_exp2f_1u (float32x4_t);
__vpcs float32x4_t _ZGVnN4v_expf (float32x4_t);
__vpcs float32x4_t _ZGVnN4v_expf_1u (float32x4_t);
__vpcs float32x4_t _ZGVnN4v_expm1f (float32x4_t);
__vpcs float32x4_t _ZGVnN4v_log10f (float32x4_t);
__vpcs float32x4_t _ZGVnN4v_log1pf (float32x4_t);
__vpcs float32x4_t _ZGVnN4v_log2f (float32x4_t);
__vpcs float32x4_t _ZGVnN4v_logf (float32x4_t);
__vpcs float32x4_t _ZGVnN4v_sinf (float32x4_t);
__vpcs float32x4_t _ZGVnN4v_sinhf (float32x4_t);
__vpcs float32x4_t _ZGVnN4v_sinpif (float32x4_t);
__vpcs float32x4_t _ZGVnN4v_tanf (float32x4_t);
__vpcs float32x4_t _ZGVnN4v_tanhf (float32x4_t);
__vpcs float32x4_t _ZGVnN4v_tanpif (float32x4_t);
__vpcs float32x4_t _ZGVnN4vl4_modff (float32x4_t, float *);
__vpcs float32x4_t _ZGVnN4vv_atan2f (float32x4_t, float32x4_t);
__vpcs float32x4_t _ZGVnN4vv_hypotf (float32x4_t, float32x4_t);
__vpcs float32x4_t _ZGVnN4vv_powf (float32x4_t, float32x4_t);
__vpcs float32x4x2_t _ZGVnN4v_cexpif (float32x4_t);
__vpcs void _ZGVnN4vl4l4_sincosf (float32x4_t, float *, float *);
__vpcs void _ZGVnN4vl4l4_sincospif (float32x4_t, float *, float *);

__vpcs float64x2_t _ZGVnN2v_acos (float64x2_t);
__vpcs float64x2_t _ZGVnN2v_acosh (float64x2_t);
__vpcs float64x2_t _ZGVnN2v_asin (float64x2_t);
__vpcs float64x2_t _ZGVnN2v_asinh (float64x2_t);
__vpcs float64x2_t _ZGVnN2v_atan (float64x2_t);
__vpcs float64x2_t _ZGVnN2v_atanh (float64x2_t);
__vpcs float64x2_t _ZGVnN2v_cbrt (float64x2_t);
__vpcs float64x2_t _ZGVnN2v_cos (float64x2_t);
__vpcs float64x2_t _ZGVnN2v_cosh (float64x2_t);
__vpcs float64x2_t _ZGVnN2v_cospi (float64x2_t);
__vpcs float64x2_t _ZGVnN2v_erf (float64x2_t);
__vpcs float64x2_t _ZGVnN2v_erfc (float64x2_t);
__vpcs float64x2_t _ZGVnN2v_exp (float64x2_t);
__vpcs float64x2_t _ZGVnN2v_exp10 (float64x2_t);
__vpcs float64x2_t _ZGVnN2v_exp2 (float64x2_t);
__vpcs float64x2_t _ZGVnN2v_expm1 (float64x2_t);
__vpcs float64x2_t _ZGVnN2v_log (float64x2_t);
__vpcs float64x2_t _ZGVnN2v_log10 (float64x2_t);
__vpcs float64x2_t _ZGVnN2v_log1p (float64x2_t);
__vpcs float64x2_t _ZGVnN2v_log2 (float64x2_t);
__vpcs float64x2_t _ZGVnN2v_sin (float64x2_t);
__vpcs float64x2_t _ZGVnN2v_sinh (float64x2_t);
__vpcs float64x2_t _ZGVnN2v_sinpi (float64x2_t);
__vpcs float64x2_t _ZGVnN2v_tan (float64x2_t);
__vpcs float64x2_t _ZGVnN2v_tanh (float64x2_t);
__vpcs float64x2_t _ZGVnN2v_tanpi (float64x2_t);
__vpcs float64x2_t _ZGVnN2vl8_modf (float64x2_t, double *);
__vpcs float64x2_t _ZGVnN2vv_atan2 (float64x2_t, float64x2_t);
__vpcs float64x2_t _ZGVnN2vv_hypot (float64x2_t, float64x2_t);
__vpcs float64x2_t _ZGVnN2vv_pow (float64x2_t, float64x2_t);
__vpcs float64x2x2_t _ZGVnN2v_cexpi (float64x2_t);
__vpcs void _ZGVnN2vl8l8_sincos (float64x2_t, double *, double *);
__vpcs void _ZGVnN2vl8l8_sincospi (float64x2_t, double *, double *);

# if WANT_EXPERIMENTAL_MATH
__vpcs float32x4_t _ZGVnN4v_erfinvf (float32x4_t);
__vpcs float64x2_t _ZGVnN2v_erfinv (float64x2_t);
# endif

# ifdef __ARM_FEATURE_SVE
#  include <arm_sve.h>
svfloat32_t _ZGVsMxv_acosf (svfloat32_t, svbool_t);
svfloat32_t _ZGVsMxv_acoshf (svfloat32_t, svbool_t);
svfloat32_t _ZGVsMxv_asinf (svfloat32_t, svbool_t);
svfloat32_t _ZGVsMxv_asinhf (svfloat32_t, svbool_t);
svfloat32_t _ZGVsMxv_atanf (svfloat32_t, svbool_t);
svfloat32_t _ZGVsMxv_atanhf (svfloat32_t, svbool_t);
svfloat32_t _ZGVsMxv_cbrtf (svfloat32_t, svbool_t);
svfloat32_t _ZGVsMxv_cosf (svfloat32_t, svbool_t);
svfloat32_t _ZGVsMxv_coshf (svfloat32_t, svbool_t);
svfloat32_t _ZGVsMxv_cospif (svfloat32_t, svbool_t);
svfloat32_t _ZGVsMxv_erfcf (svfloat32_t, svbool_t);
svfloat32_t _ZGVsMxv_erff (svfloat32_t, svbool_t);
svfloat32_t _ZGVsMxv_exp10f (svfloat32_t, svbool_t);
svfloat32_t _ZGVsMxv_exp2f (svfloat32_t, svbool_t);
svfloat32_t _ZGVsMxv_expf (svfloat32_t, svbool_t);
svfloat32_t _ZGVsMxv_expm1f (svfloat32_t, svbool_t);
svfloat32_t _ZGVsMxv_log10f (svfloat32_t, svbool_t);
svfloat32_t _ZGVsMxv_log1pf (svfloat32_t, svbool_t);
svfloat32_t _ZGVsMxv_log2f (svfloat32_t, svbool_t);
svfloat32_t _ZGVsMxv_logf (svfloat32_t, svbool_t);
svfloat32_t _ZGVsMxv_sinf (svfloat32_t, svbool_t);
svfloat32_t _ZGVsMxv_sinhf (svfloat32_t, svbool_t);
svfloat32_t _ZGVsMxv_sinpif (svfloat32_t, svbool_t);
svfloat32_t _ZGVsMxv_tanf (svfloat32_t, svbool_t);
svfloat32_t _ZGVsMxv_tanhf (svfloat32_t, svbool_t);
svfloat32_t _ZGVsMxv_tanpif (svfloat32_t, svbool_t);
svfloat32_t _ZGVsMxvl4_modff (svfloat32_t, float *, svbool_t);
svfloat32_t _ZGVsMxvv_atan2f (svfloat32_t, svfloat32_t, svbool_t);
svfloat32_t _ZGVsMxvv_hypotf (svfloat32_t, svfloat32_t, svbool_t);
svfloat32_t _ZGVsMxvv_powf (svfloat32_t, svfloat32_t, svbool_t);
svfloat32x2_t _ZGVsMxv_cexpif (svfloat32_t, svbool_t);
void _ZGVsMxvl4l4_sincosf (svfloat32_t, float *, float *, svbool_t);
void _ZGVsMxvl4l4_sincospif (svfloat32_t, float *, float *, svbool_t);

svfloat64_t _ZGVsMxv_acos (svfloat64_t, svbool_t);
svfloat64_t _ZGVsMxv_acosh (svfloat64_t, svbool_t);
svfloat64_t _ZGVsMxv_asin (svfloat64_t, svbool_t);
svfloat64_t _ZGVsMxv_asinh (svfloat64_t, svbool_t);
svfloat64_t _ZGVsMxv_atan (svfloat64_t, svbool_t);
svfloat64_t _ZGVsMxv_atanh (svfloat64_t, svbool_t);
svfloat64_t _ZGVsMxv_cbrt (svfloat64_t, svbool_t);
svfloat64_t _ZGVsMxv_cos (svfloat64_t, svbool_t);
svfloat64_t _ZGVsMxv_cosh (svfloat64_t, svbool_t);
svfloat64_t _ZGVsMxv_cospi (svfloat64_t, svbool_t);
svfloat64_t _ZGVsMxv_erf (svfloat64_t, svbool_t);
svfloat64_t _ZGVsMxv_erfc (svfloat64_t, svbool_t);
svfloat64_t _ZGVsMxv_exp (svfloat64_t, svbool_t);
svfloat64_t _ZGVsMxv_exp10 (svfloat64_t, svbool_t);
svfloat64_t _ZGVsMxv_exp2 (svfloat64_t, svbool_t);
svfloat64_t _ZGVsMxv_expm1 (svfloat64_t, svbool_t);
svfloat64_t _ZGVsMxv_log (svfloat64_t, svbool_t);
svfloat64_t _ZGVsMxv_log10 (svfloat64_t, svbool_t);
svfloat64_t _ZGVsMxv_log1p (svfloat64_t, svbool_t);
svfloat64_t _ZGVsMxv_log2 (svfloat64_t, svbool_t);
svfloat64_t _ZGVsMxv_sin (svfloat64_t, svbool_t);
svfloat64_t _ZGVsMxv_sinh (svfloat64_t, svbool_t);
svfloat64_t _ZGVsMxv_sinpi (svfloat64_t, svbool_t);
svfloat64_t _ZGVsMxv_tan (svfloat64_t, svbool_t);
svfloat64_t _ZGVsMxv_tanh (svfloat64_t, svbool_t);
svfloat64_t _ZGVsMxv_tanpi (svfloat64_t, svbool_t);
svfloat64_t _ZGVsMxvl8_modf (svfloat64_t, double *, svbool_t);
svfloat64_t _ZGVsMxvv_atan2 (svfloat64_t, svfloat64_t, svbool_t);
svfloat64_t _ZGVsMxvv_hypot (svfloat64_t, svfloat64_t, svbool_t);
svfloat64_t _ZGVsMxvv_pow (svfloat64_t, svfloat64_t, svbool_t);
svfloat64x2_t _ZGVsMxv_cexpi (svfloat64_t, svbool_t);
void _ZGVsMxvl8l8_sincos (svfloat64_t, double *, double *, svbool_t);
void _ZGVsMxvl8l8_sincospi (svfloat64_t, double *, double *, svbool_t);

#  if WANT_EXPERIMENTAL_MATH

svfloat32_t _ZGVsMxv_erfinvf (svfloat32_t, svbool_t);
svfloat32_t _ZGVsMxvv_powi (svfloat32_t, svint32_t, svbool_t);

svfloat64_t _ZGVsMxvv_powk (svfloat64_t, svint64_t, svbool_t);
svfloat64_t _ZGVsMxv_erfinv (svfloat64_t, svbool_t);

#  endif
# endif
#endif

#endif
