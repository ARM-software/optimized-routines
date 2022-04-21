/*
 * Public API.
 *
 * Copyright (c) 2015-2022, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#ifndef _MATHLIB_H
#define _MATHLIB_H

float erfcf(float);
float erff (float);
float log10f (float);

double log10 (double);

float __s_log10f (float);
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
__f32x4_t __v_log10f (__f32x4_t);
__f64x2_t __v_log10 (__f64x2_t);

#if __GNUC__ >= 9 || __clang_major__ >= 8
#define __vpcs __attribute__((__aarch64_vector_pcs__))

/* Vector functions following the vector PCS.  */
__vpcs __f32x4_t __vn_log10f (__f32x4_t);
__vpcs __f64x2_t __vn_log10 (__f64x2_t);

/* Vector functions following the vector PCS using ABI names.  */
__vpcs __f32x4_t _ZGVnN4v_log10f (__f32x4_t);
__vpcs __f64x2_t _ZGVnN2v_log10 (__f64x2_t);

#endif
#endif

#endif
