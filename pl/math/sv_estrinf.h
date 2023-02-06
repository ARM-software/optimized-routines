/*
 * Helper macros for single-precision Estrin polynomial evaluation.
 *
 * Copyright (c) 2023, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#if SV_SUPPORTED
#define FMA sv_fma_f32_x
#endif

#include "sv_estrin_wrap.h"
