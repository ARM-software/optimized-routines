/*
 * Coefficients for double-precision vector e^x function.
 *
 * Copyright (c) 2023, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "math_config.h"

/* Coefficients copied from the polynomial for scalar exp in math/,
   with  N == 64 && EXP_POLY_ORDER == 5 && !EXP_POLY_WIDE.  */
const double __sv_exp_poly[] = {0x1.fffffffffdbcdp-2, 0x1.555555555444cp-3,
				0x1.555573c6a9f7dp-5, 0x1.1111266d28935p-7};
