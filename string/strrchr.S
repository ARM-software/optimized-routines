/*
 * Selected possible strrchr implementations.
 *
 * Copyright (c) 2019, Arm Limited.
 * SPDX-License-Identifier: MIT
 */

#if __aarch64__
#include "aarch64/strrchr.S"
# if __ARM_FEATURE_SVE
#include "aarch64/strrchr-sve.S"
# endif
#endif
