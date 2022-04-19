/*
 * Declarations for double-precision log10(x) vector function.
 *
 * Copyright (c) 2019-2022, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "v_math.h"
#if WANT_VMATH

#define V_LOG10_TABLE_BITS 7

extern const struct v_log10_data
{
  f64_t invc;
  f64_t log10c;
} __v_log10_data[1 << V_LOG10_TABLE_BITS] HIDDEN;

#endif
