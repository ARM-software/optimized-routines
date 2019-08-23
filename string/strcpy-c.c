/*
 * Selected possible strcpy implementations.
 *
 * Copyright (c) 2019, Arm Limited.
 * SPDX-License-Identifier: MIT
 */

#if __arm__ && defined (__thumb2__) && !defined (__thumb__)
#include "arm/strcpy.c"
#endif
