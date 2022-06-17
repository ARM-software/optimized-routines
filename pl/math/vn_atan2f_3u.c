/*
 * AdvSIMD vector PCS variant of __v_atan2f.
 *
 * Copyright (c) 2019-2022, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */
#include "include/mathlib.h"
#ifdef __vpcs
#define VPCS 1
#define VPCS_ALIAS strong_alias (__vn_atan2f, _ZGVnN4vv_atan2f)
#include "v_atan2f_3u.c"
#endif
