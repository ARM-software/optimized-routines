/*
 * Selected possible memchr implementations.
 *
 * Copyright (c) 2019, Arm Limited.
 * SPDX-License-Identifier: MIT
 */

#if __aarch64__
#include "aarch64/memchr.S"
#elif __arm__
#include "arm/memchr.S"
#endif
