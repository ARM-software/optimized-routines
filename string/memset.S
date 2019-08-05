/*
 * Selected possible memset implementations.
 *
 * Copyright (c) 2019, Arm Limited.
 * SPDX-License-Identifier: MIT
 */

#if __aarch64__
#include "aarch64/memset.S"
#elif __arm__
#include "arm/memset.S"
#endif
