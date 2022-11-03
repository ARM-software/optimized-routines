// clang-format off
/*
 * Public API.
 *
 * Copyright (c) 2015-2022, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#ifndef _MATHLIB_H
#define _MATHLIB_H

float acoshf (float);
float asinhf (float);
float atan2f (float, float);
float erfcf (float);
float erff (float);
float expm1f (float);
float log10f (float);
float log1pf (float);
float sinhf (float);
float tanf (float);

double acosh (double);
double asinh (double);
double atan2 (double, double);
double erfc (double);
double log10 (double);
double log1p (double);

float __s_asinhf (float);
float __s_atanf (float);
float __s_atan2f (float, float);
float __s_erfcf (float);
float __s_erff (float);
float __s_expm1f (float);
float __s_log10f (float);
float __s_log1pf (float);
float __s_log2f (float);
float __s_tanf (float);

double __s_atan (double);
double __s_atan2 (double, double);
double __s_erf (double);
double __s_erfc (double);
double __s_log10 (double);
double __s_log1p (double);
double __s_log2 (double);

#if __aarch64__
#if __GNUC__ >= 5
typedef __Float32x4_t __f32x4_t;
typedef __Float64x2_t __f64x2_t;
#elif __clang_major__*100+__clang_minor__ >= 305
typedef __attribute__((__neon_vector_type__(4))) float __f32x4_t;
typedef __attribute__((__neon_vector_type__(2))) double __f64x2_t;
#else
#error Unsupported compiler
#endif

/* Vector functions following the base PCS.  */
__f32x4_t __v_asinhf (__f32x4_t);
__f32x4_t __v_atanf (__f32x4_t);
__f64x2_t __v_atan (__f64x2_t);
__f32x4_t __v_atan2f (__f32x4_t, __f32x4_t);
__f64x2_t __v_atan2 (__f64x2_t, __f64x2_t);
__f32x4_t __v_erff (__f32x4_t);
__f64x2_t __v_erf (__f64x2_t);
__f32x4_t __v_erfcf (__f32x4_t);
__f64x2_t __v_erfc (__f64x2_t);
__f32x4_t __v_expm1f (__f32x4_t);
__f32x4_t __v_log10f (__f32x4_t);
__f64x2_t __v_log10 (__f64x2_t);
__f32x4_t __v_log1pf (__f32x4_t);
__f64x2_t __v_log1p (__f64x2_t);
__f32x4_t __v_log2f (__f32x4_t);
__f64x2_t __v_log2 (__f64x2_t);
__f32x4_t __v_tanf (__f32x4_t);

#if __GNUC__ >= 9 || __clang_major__ >= 8
#define __vpcs __attribute__((__aarch64_vector_pcs__))

/* Vector functions following the vector PCS.  */
__vpcs __f32x4_t __vn_asinhf (__f32x4_t);
__vpcs __f32x4_t __vn_atanf (__f32x4_t);
__vpcs __f64x2_t __vn_atan (__f64x2_t);
__vpcs __f32x4_t __vn_atan2f (__f32x4_t, __f32x4_t);
__vpcs __f64x2_t __vn_atan2 (__f64x2_t, __f64x2_t);
__vpcs __f32x4_t __vn_erff (__f32x4_t);
__vpcs __f64x2_t __vn_erf (__f64x2_t);
__vpcs __f32x4_t __vn_erfcf (__f32x4_t);
__vpcs __f64x2_t __vn_erfc (__f64x2_t);
__vpcs __f32x4_t __vn_expm1f (__f32x4_t);
__vpcs __f32x4_t __vn_log10f (__f32x4_t);
__vpcs __f64x2_t __vn_log10 (__f64x2_t);
__vpcs __f32x4_t __vn_log1pf (__f32x4_t);
__vpcs __f64x2_t __vn_log1p (__f64x2_t);
__vpcs __f32x4_t __vn_log2f (__f32x4_t);
__vpcs __f64x2_t __vn_log2 (__f64x2_t);
__vpcs __f32x4_t __vn_tanf (__f32x4_t);

/* Vector functions following the vector PCS using ABI names.  */
__vpcs __f32x4_t _ZGVnN4v_asinhf (__f32x4_t);
__vpcs __f32x4_t _ZGVnN4v_atanf (__f32x4_t);
__vpcs __f64x2_t _ZGVnN2v_atan (__f64x2_t);
__vpcs __f32x4_t _ZGVnN4vv_atan2f (__f32x4_t, __f32x4_t);
__vpcs __f64x2_t _ZGVnN2vv_atan2 (__f64x2_t, __f64x2_t);
__vpcs __f32x4_t _ZGVnN4v_erff (__f32x4_t);
__vpcs __f64x2_t _ZGVnN2v_erf (__f64x2_t);
__vpcs __f32x4_t _ZGVnN4v_erfcf (__f32x4_t);
__vpcs __f64x2_t _ZGVnN2v_erfc (__f64x2_t);
__vpcs __f32x4_t _ZGVnN4v_expm1f (__f32x4_t);
__vpcs __f32x4_t _ZGVnN4v_log10f (__f32x4_t);
__vpcs __f64x2_t _ZGVnN2v_log10 (__f64x2_t);
__vpcs __f32x4_t _ZGVnN4v_log1pf (__f32x4_t);
__vpcs __f64x2_t _ZGVnN2v_log1p (__f64x2_t);
__vpcs __f32x4_t _ZGVnN4v_log2f (__f32x4_t);
__vpcs __f64x2_t _ZGVnN2v_log2 (__f64x2_t);
__vpcs __f32x4_t _ZGVnN4v_tanf (__f32x4_t);

#endif

#if WANT_SVE_MATH
#include <arm_sve.h>
svfloat32_t __sv_atan2f_x (svfloat32_t, svfloat32_t, svbool_t);
svfloat32_t __sv_atanf_x (svfloat32_t, svbool_t);
svfloat64_t __sv_atan_x (svfloat64_t, svbool_t);
svfloat64_t __sv_atan2_x (svfloat64_t, svfloat64_t, svbool_t);
svfloat32_t __sv_cosf_x (svfloat32_t, svbool_t);
svfloat64_t __sv_cos_x (svfloat64_t, svbool_t);
svfloat32_t __sv_erff_x (svfloat32_t, svbool_t);
svfloat64_t __sv_erf_x (svfloat64_t, svbool_t);
svfloat64_t __sv_erfc_x (svfloat64_t, svbool_t);
svfloat32_t __sv_expf_x (svfloat32_t, svbool_t);
svfloat32_t __sv_logf_x (svfloat32_t, svbool_t);
svfloat64_t __sv_log_x (svfloat64_t, svbool_t);
svfloat32_t __sv_log10f_x (svfloat32_t, svbool_t);
svfloat64_t __sv_log10_x (svfloat64_t, svbool_t);
svfloat32_t __sv_powif_x (svfloat32_t, svint32_t, svbool_t);
svfloat64_t __sv_powi_x (svfloat64_t, svint64_t, svbool_t);
svfloat32_t __sv_sinf_x (svfloat32_t, svbool_t);
svfloat64_t __sv_sin_x (svfloat64_t, svbool_t);
svfloat32_t __sv_tanf_x (svfloat32_t, svbool_t);
/* SVE ABI names.  */
svfloat32_t _ZGVsMxvv_atan2f (svfloat32_t, svfloat32_t, svbool_t);
svfloat32_t _ZGVsMxv_atanf (svfloat32_t, svbool_t);
svfloat64_t _ZGVsMxv_atan (svfloat64_t, svbool_t);
svfloat64_t _ZGVsMxvv_atan2 (svfloat64_t, svfloat64_t, svbool_t);
svfloat32_t _ZGVsMxv_cosf (svfloat32_t, svbool_t);
svfloat64_t _ZGVsMxv_cos (svfloat64_t, svbool_t);
svfloat32_t _ZGVsMxv_erff (svfloat32_t, svbool_t);
svfloat64_t _ZGVsMxv_erf (svfloat64_t, svbool_t);
svfloat64_t _ZGVsMxv_erfc (svfloat64_t, svbool_t);
svfloat32_t _ZGVsMxv_expf (svfloat32_t, svbool_t);
svfloat32_t _ZGVsMxv_logf (svfloat32_t, svbool_t);
svfloat64_t _ZGVsMxv_log (svfloat64_t, svbool_t);
svfloat32_t _ZGVsMxv_log10f (svfloat32_t, svbool_t);
svfloat64_t _ZGVsMxv_log10 (svfloat64_t, svbool_t);
svfloat32_t _ZGVsMxvv_powi(svfloat32_t, svint32_t, svbool_t);
svfloat64_t _ZGVsMxvv_powk(svfloat64_t, svint64_t, svbool_t);
svfloat32_t _ZGVsMxv_sinf (svfloat32_t, svbool_t);
svfloat64_t _ZGVsMxv_sin (svfloat64_t, svbool_t);
svfloat32_t _ZGVsMxv_tanf (svfloat32_t, svbool_t);
#endif

#endif

#endif
// clang-format on
