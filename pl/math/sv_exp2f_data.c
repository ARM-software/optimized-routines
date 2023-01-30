/*
 * Coefficients for single-precision vector 2^x function.
 *
 * Copyright (c) 2023, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "math_config.h"

/* Coefficients copied from the polynomial in math/v_exp2f.c.  */
const float __sv_exp2f_poly[]
  = {0x1.59977ap-10f, 0x1.3ce9e4p-7f, 0x1.c6bd32p-5f, 0x1.ebf9bcp-3f,
     0x1.62e422p-1f};