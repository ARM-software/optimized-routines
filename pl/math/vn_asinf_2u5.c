/*
 * AdvSIMD vector PCS variant of __v_asinf.
 *
 * Copyright (c) 2023, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */
#include "include/mathlib.h"
#ifdef __vpcs
#define VPCS 1
#define VPCS_ALIAS strong_alias (__vn_asinf, _ZGVnN4v_asinf)
#include "v_asinf_2u5.c"
#endif
