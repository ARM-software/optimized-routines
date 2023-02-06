/*
 * Helper macros for double-precision Estrin polynomial evaluation.
 *
 * Copyright (c) 2023, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#if SV_SUPPORTED
#define FMA sv_fma_f64_x
#endif

#include "sv_estrin_wrap.h"
