/*
 * Coefficients for vector log2f
 *
 * Copyright (c) 2022-2023, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "math_config.h"

const float __sv_log2f_poly[] = {
  /* See tools/v_log2f.sollya for the algorithm used to generate these
     coefficients.  */
  0x1.715476p0f, /* (float)(1 / ln(2)).  */
  -0x1.715458p-1f, 0x1.ec701cp-2f, -0x1.7171a4p-2f, 0x1.27a0b8p-2f,
  -0x1.e5143ep-3f, 0x1.9d8ecap-3f, -0x1.c675bp-3f,  0x1.9e495p-3f
};
