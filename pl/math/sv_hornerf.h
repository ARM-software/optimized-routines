/*
 * Helper macros for single-precision Horner polynomial evaluation
 * in SVE routines.
 *
 * Copyright (c) 2023, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#define FMA(pg, x, y, z) svmla_f32_x (pg, z, x, y)
#define VECTOR sv_f32

#include "sv_horner_wrap.h"
