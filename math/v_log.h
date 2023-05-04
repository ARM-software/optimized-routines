/*
 * Declarations for double-precision log(x) vector function.
 *
 * Copyright (c) 2019-2023, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "v_math.h"
#if WANT_VMATH

#define V_LOG_TABLE_BITS 7

extern const struct v_log_data
{
  double invc;
  double logc;
} __v_log_data[1 << V_LOG_TABLE_BITS] HIDDEN;
#endif
