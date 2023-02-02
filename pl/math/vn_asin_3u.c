/*
 * AdvSIMD vector PCS variant of __v_asin.
 *
 * Copyright (c) 2023, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */
#include "include/mathlib.h"
#ifdef __vpcs
#define VPCS 1
#define VPCS_ALIAS strong_alias (__vn_asin, _ZGVnN2v_asin)
#include "v_asin_3u.c"
#endif
