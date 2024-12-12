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

#if __aarch64__ && __linux__
# if __GNUC__ >= 5
typedef __Float32x4_t __f32x4_t;
typedef __Float64x2_t __f64x2_t;
# elif __clang_major__*100+__clang_minor__ >= 305
typedef __attribute__((__neon_vector_type__(4))) float __f32x4_t;
typedef __attribute__((__neon_vector_type__(2))) double __f64x2_t;
# else
#  error Unsupported compiler
# endif

# if __GNUC__ >= 9 || __clang_major__ >= 8
#  undef __vpcs
#  define __vpcs __attribute__((__aarch64_vector_pcs__))

/* Vector functions following the vector PCS using ABI names.  */
__vpcs __f32x4_t _ZGVnN4v_acosf (__f32x4_t);
__vpcs __f32x4_t _ZGVnN4v_acoshf (__f32x4_t);
__vpcs __f32x4_t _ZGVnN4v_asinf (__f32x4_t);
__vpcs __f32x4_t _ZGVnN4v_asinhf (__f32x4_t);
__vpcs __f32x4_t _ZGVnN4v_atanf (__f32x4_t);
__vpcs __f32x4_t _ZGVnN4v_atanhf (__f32x4_t);
__vpcs __f32x4_t _ZGVnN4v_cbrtf (__f32x4_t);
__vpcs __f32x4_t _ZGVnN4v_cosf (__f32x4_t);
__vpcs __f32x4_t _ZGVnN4v_coshf (__f32x4_t);
__vpcs __f32x4_t _ZGVnN4v_cospif (__f32x4_t);
__vpcs __f32x4_t _ZGVnN4v_erfcf (__f32x4_t);
__vpcs __f32x4_t _ZGVnN4v_erff (__f32x4_t);
__vpcs __f32x4_t _ZGVnN4v_exp10f (__f32x4_t);
__vpcs __f32x4_t _ZGVnN4v_exp2f (__f32x4_t);
__vpcs __f32x4_t _ZGVnN4v_exp2f_1u (__f32x4_t);
__vpcs __f32x4_t _ZGVnN4v_expf (__f32x4_t);
__vpcs __f32x4_t _ZGVnN4v_expf_1u (__f32x4_t);
__vpcs __f32x4_t _ZGVnN4v_expm1f (__f32x4_t);
__vpcs __f32x4_t _ZGVnN4v_log10f (__f32x4_t);
__vpcs __f32x4_t _ZGVnN4v_log1pf (__f32x4_t);
__vpcs __f32x4_t _ZGVnN4v_log2f (__f32x4_t);
__vpcs __f32x4_t _ZGVnN4v_logf (__f32x4_t);
__vpcs __f32x4_t _ZGVnN4v_sinf (__f32x4_t);
__vpcs __f32x4_t _ZGVnN4v_sinhf (__f32x4_t);
__vpcs __f32x4_t _ZGVnN4v_sinpif (__f32x4_t);
__vpcs __f32x4_t _ZGVnN4v_tanf (__f32x4_t);
__vpcs __f32x4_t _ZGVnN4v_tanhf (__f32x4_t);
__vpcs __f32x4_t _ZGVnN4v_tanpif (__f32x4_t);
__vpcs __f32x4_t _ZGVnN4vv_atan2f (__f32x4_t, __f32x4_t);
__vpcs __f32x4_t _ZGVnN4vv_hypotf (__f32x4_t, __f32x4_t);
__vpcs __f32x4_t _ZGVnN4vv_powf (__f32x4_t, __f32x4_t);
__vpcs void _ZGVnN4vl4l4_sincospif (__f32x4_t, __f32x4_t *, __f32x4_t *);

__vpcs __f64x2_t _ZGVnN2v_acos (__f64x2_t);
__vpcs __f64x2_t _ZGVnN2v_acosh (__f64x2_t);
__vpcs __f64x2_t _ZGVnN2v_asin (__f64x2_t);
__vpcs __f64x2_t _ZGVnN2v_asinh (__f64x2_t);
__vpcs __f64x2_t _ZGVnN2v_atan (__f64x2_t);
__vpcs __f64x2_t _ZGVnN2v_atanh (__f64x2_t);
__vpcs __f64x2_t _ZGVnN2v_cbrt (__f64x2_t);
__vpcs __f64x2_t _ZGVnN2v_cos (__f64x2_t);
__vpcs __f64x2_t _ZGVnN2v_cosh (__f64x2_t);
__vpcs __f64x2_t _ZGVnN2v_cospi (__f64x2_t);
__vpcs __f64x2_t _ZGVnN2v_erf (__f64x2_t);
__vpcs __f64x2_t _ZGVnN2v_erfc (__f64x2_t);
__vpcs __f64x2_t _ZGVnN2v_exp10 (__f64x2_t);
__vpcs __f64x2_t _ZGVnN2v_exp2 (__f64x2_t);
__vpcs __f64x2_t _ZGVnN2v_exp (__f64x2_t);
__vpcs __f64x2_t _ZGVnN2v_expm1 (__f64x2_t);
__vpcs __f64x2_t _ZGVnN2v_log (__f64x2_t);
__vpcs __f64x2_t _ZGVnN2v_log10 (__f64x2_t);
__vpcs __f64x2_t _ZGVnN2v_log1p (__f64x2_t);
__vpcs __f64x2_t _ZGVnN2v_log2 (__f64x2_t);
__vpcs __f64x2_t _ZGVnN2v_sin (__f64x2_t);
__vpcs __f64x2_t _ZGVnN2v_sinh (__f64x2_t);
__vpcs __f64x2_t _ZGVnN2v_sinpi (__f64x2_t);
__vpcs __f64x2_t _ZGVnN2v_tan (__f64x2_t);
__vpcs __f64x2_t _ZGVnN2v_tanh (__f64x2_t);
__vpcs __f64x2_t _ZGVnN2v_tanpi (__f64x2_t);
__vpcs __f64x2_t _ZGVnN2vv_atan2 (__f64x2_t, __f64x2_t);
__vpcs __f64x2_t _ZGVnN2vv_hypot (__f64x2_t, __f64x2_t);
__vpcs __f64x2_t _ZGVnN2vv_pow (__f64x2_t, __f64x2_t);
__vpcs void _ZGVnN2vl8l8_sincospi (__f64x2_t, __f64x2_t *, __f64x2_t *);
# endif

# if WANT_SVE_MATH
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
svfloat32_t _ZGVsMxvv_atan2f (svfloat32_t, svfloat32_t, svbool_t);
svfloat32_t _ZGVsMxvv_hypotf (svfloat32_t, svfloat32_t, svbool_t);
svfloat32_t _ZGVsMxvv_powf (svfloat32_t, svfloat32_t, svbool_t);
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
svfloat64_t _ZGVsMxvv_atan2 (svfloat64_t, svfloat64_t, svbool_t);
svfloat64_t _ZGVsMxvv_hypot (svfloat64_t, svfloat64_t, svbool_t);
svfloat64_t _ZGVsMxvv_pow (svfloat64_t, svfloat64_t, svbool_t);
void _ZGVsMxvl8l8_sincospi (svfloat64_t, double *, double *, svbool_t);
# endif
#endif

#endif
