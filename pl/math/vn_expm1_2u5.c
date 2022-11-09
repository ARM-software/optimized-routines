/*
 * AdvSIMD vector PCS variant of __v_expm1.
 *
 * Copyright (c) 2022, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */
#include "include/mathlib.h"
#ifdef __vpcs
#define VPCS 1
#define VPCS_ALIAS strong_alias (__vn_expm1, _ZGVnN2v_expm1)
#include "v_expm1_2u5.c"
#endif
