/*
 * Helper macros for double-precision pairwise Horner polynomial evaluation
 * in SVE routines
 *
 * Copyright (c) 2023, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#if SV_SUPPORTED
#define FMA(pg, x, y, z) svmla_f64_x (pg, z, x, y)
#define VECTOR sv_f64
#endif

#include "sv_pairwise_horner_wrap.h"
