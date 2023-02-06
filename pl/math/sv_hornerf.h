/*
 * Helper macros for single-precision Horner polynomial evaluation
 * in SVE routines.
 *
 * Copyright (c) 2023, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#if SV_SUPPORTED
#define FMA sv_fma_f32_x
#define VECTOR sv_f32
#endif

#include "sv_horner_wrap.h"
