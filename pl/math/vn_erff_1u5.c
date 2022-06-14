/*
 * AdvSIMD vector PCS variant of __v_erff.
 *
 * Copyright (c) 2019-2022, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */
#include "include/mathlib.h"
#ifdef __vpcs
#define VPCS 1
#define VPCS_ALIAS strong_alias (__vn_erff, _ZGVnN4v_erff);
#include "v_erff_1u5.c"
#endif
