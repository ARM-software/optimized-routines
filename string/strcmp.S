/*
 * Selected possible strcmp implementations.
 *
 * Copyright (c) 2019, Arm Limited.
 * SPDX-License-Identifier: MIT
 */

#if __aarch64__
#include "aarch64/strcmp.S"
#elif __arm__ && __ARM_ARCH >= 7 && __ARM_ARCH_ISA_ARM >= 1
#include "arm/strcmp.S"
#endif
