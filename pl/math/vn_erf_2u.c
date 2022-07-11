/*
 * AdvSIMD vector PCS variant of __v_erf.
 *
 * Copyright (c) 2019-2022, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */
#include "include/mathlib.h"
#ifdef __vpcs
#define VPCS 1
#define VPCS_ALIAS strong_alias (__vn_erf, _ZGVnN2v_erf)
#include "v_erf_2u.c"
#endif
