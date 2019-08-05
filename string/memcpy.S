/*
 * Selected possible memcpy implementations.
 *
 * Copyright (c) 2019, Arm Limited.
 * SPDX-License-Identifier: MIT
 */

#if __aarch64__
#include "aarch64/memcpy.S"
#elif __arm__
#include "arm/memcpy.S"
#endif
