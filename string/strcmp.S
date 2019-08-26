/*
 * Selected possible strcmp implementations.
 *
 * Copyright (c) 2019, Arm Limited.
 * SPDX-License-Identifier: MIT
 */

#if __aarch64__
#include "aarch64/strcmp.S"
# if __ARM_FEATURE_SVE
#include "aarch64/strcmp-sve.S"
# endif
#elif __arm__
# if __ARM_ARCH >= 7 && __ARM_ARCH_ISA_ARM >= 1
#include "arm/strcmp.S"
# elif __ARM_ARCH == 6 && __ARM_ARCH_6M__ >= 1
#include "arm/strcmp-armv6m.S"
# endif
#endif
