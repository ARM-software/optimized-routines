/*
 * Selected possible strlen implementations.
 *
 * Copyright (c) 2019, Arm Limited.
 * SPDX-License-Identifier: MIT
 */

#if __aarch64__
#include "aarch64/strlen.S"
# if __ARM_FEATURE_SVE
#include "aarch64/strlen-sve.S"
# endif
#elif __arm__
# if __ARM_ARCH >= 6 && __ARM_ARCH_ISA_THUMB == 2
#include "arm/strlen-armv6t2.S"
# endif
#endif
