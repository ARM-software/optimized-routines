/*
 * Helper macros for single-precision Estrin polynomial evaluation.
 *
 * Copyright (c) 2023, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#define FMA(pg, x, y, z) svmla_f32_x (pg, z, x, y)

#include "sv_estrin_wrap.h"
