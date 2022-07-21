// clang-format off
/*
 * Public API.
 *
 * Copyright (c) 2015-2022, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#ifndef _MATHLIB_H
#define _MATHLIB_H

float asinhf (float);
float atan2f (float, float);
float erfcf (float);
float erff (float);
float log10f (float);
float log1pf (float);

double asinh (double);
double atan2 (double, double);
double log10 (double);
double log1p (double);

float __s_asinhf (float);
float __s_atanf (float);
float __s_atan2f (float, float);
float __s_erfcf (float);
float __s_erff (float);
float __s_log10f (float);
float __s_log1pf (float);

double __s_atan (double);
double __s_atan2 (double, double);
double __s_erf (double);
double __s_erfc (double);
double __s_log10 (double);

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
__f32x4_t __v_log10f (__f32x4_t);
__f64x2_t __v_log10 (__f64x2_t);
__f32x4_t __v_log1pf (__f32x4_t);

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
__vpcs __f32x4_t __vn_log10f (__f32x4_t);
__vpcs __f64x2_t __vn_log10 (__f64x2_t);
__vpcs __f32x4_t __vn_log1pf (__f32x4_t);

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
__vpcs __f32x4_t _ZGVnN4v_log10f (__f32x4_t);
__vpcs __f64x2_t _ZGVnN2v_log10 (__f64x2_t);
__vpcs __f32x4_t _ZGVnN4v_log1pf (__f32x4_t);

#endif

#if WANT_SVE_MATH
#include <arm_sve.h>
svfloat32_t __sv_cosf_x (svfloat32_t, svbool_t);
svfloat64_t __sv_cos_x (svfloat64_t, svbool_t);
/* SVE ABI names.  */
svfloat32_t _ZGVsMxv_cosf (svfloat32_t, svbool_t);
svfloat64_t _ZGVsMxv_cos (svfloat64_t, svbool_t);
#endif

#endif

#endif
// clang-format on
