/*
 * AdvSIMD vector PCS variant of __v_acosf.
 *
 * Copyright (c) 2023, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */
#include "include/mathlib.h"
#ifdef __vpcs
#define VPCS 1
#define VPCS_ALIAS strong_alias (__vn_acosf, _ZGVnN4v_acosf)
#include "v_acosf_1u4.c"
#endif
