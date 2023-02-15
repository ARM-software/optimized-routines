/*
 * Helper macros for single-precision Estrin polynomial evaluation.
 *
 * Copyright (c) 2022-2023, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#if WANT_VMATH
#define FMA(x, y, z) vfmaq_f32 (z, x, y)
#else
#define FMA fmaf
#endif

#include "estrin_wrap.h"
