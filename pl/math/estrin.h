/*
 * Helper macros for double-precision Estrin polynomial evaluation.
 *
 * Copyright (c) 2022-2023, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "math_config.h"

#if WANT_VMATH
#define FMA(x, y, z) vfmaq_f64 (z, x, y)
#else
#define FMA fma
#endif

#include "estrin_wrap.h"
