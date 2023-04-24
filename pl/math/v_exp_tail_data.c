/*
 * Lookup table for double-precision e^x vector function.
 *
 * Copyright (c) 2019-2023, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "math_config.h"

#define N (1 << V_EXP_TAIL_TABLE_BITS)

/* 2^(j/N), j=0..N.  */
const uint64_t __v_exp_tail_data[] = {
#include "v_exp_data.h"
};
