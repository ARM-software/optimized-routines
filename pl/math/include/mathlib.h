/*
 * Public API.
 *
 * Copyright (c) 2015-2022, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#ifndef _MATHLIB_H
#define _MATHLIB_H

float erff (float);
float log10f (float);

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

#endif

#endif
