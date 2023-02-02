/*
 * AdvSIMD vector PCS variant of __v_acos.
 *
 * Copyright (c) 2023, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */
#include "include/mathlib.h"
#ifdef __vpcs
#define VPCS 1
#define VPCS_ALIAS strong_alias (__vn_acos, _ZGVnN2v_acos)
#include "v_acos_2u.c"
#endif
