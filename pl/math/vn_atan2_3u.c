/*
 * AdvSIMD vector PCS variant of __v_atan2.
 *
 * Copyright (c) 2019-2022, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */
#include "include/mathlib.h"
#ifdef __vpcs
#define VPCS 1
#define VPCS_ALIAS strong_alias (__vn_atan2, _ZGVnN2vv_atan2)
#include "v_atan2_3u.c"
#endif
