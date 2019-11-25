/*
 * Selected possible memcpy implementations.
 *
 * Copyright (c) 2019, Arm Limited.
 * SPDX-License-Identifier: MIT
 */

#if __aarch64__
#include "aarch64/memcpy.S"
# if __ARM_NEON
#include "aarch64/memcpy_simd.S"
# endif
#elif __arm__
#include "arm/memcpy.S"
#endif
