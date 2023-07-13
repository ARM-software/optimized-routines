/*
 * Helper macros for double-precision Estrin polynomial evaluation.
 *
 * Copyright (c) 2022-2023, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */
#include <arm_neon.h>

#define FMA(x, y, z) vfmaq_f64 (z, x, y)
#include "estrin_wrap.h"
