/*
 * AdvSIMD vector PCS variant of __v_exp2.
 *
 * Copyright (c) 2023, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */
#include "mathlib.h"
#ifdef __vpcs
#define VPCS 1
#define VPCS_ALIAS PL_ALIAS (__vn_exp2, _ZGVnN2v_exp2)
#include "v_exp2_1u5.c"
#endif
