/*
 * AdvSIMD vector PCS variant of __v_atanhf.
 *
 * Copyright (c) 2022, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */
#include "include/mathlib.h"
#ifdef __vpcs
#define VPCS 1
#define VPCS_ALIAS strong_alias (__vn_atanhf, _ZGVnN4v_atanhf)
#include "v_atanhf_3u1.c"
#endif
