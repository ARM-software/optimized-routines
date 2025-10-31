/*
 * Inline helpers for SVE modf.
 *
 * Copyright (c) 2025, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#ifndef SV_MODF_INLINE_H
#define SV_MODF_INLINE_H

#include "sv_math.h"

static inline svfloat64x2_t
sv_modf_inline (svbool_t pg, svfloat64_t x)
{
  svfloat64_t integral = svrintz_x (pg, x);
  svbool_t is_real = svcmpne (pg, x, integral);
  svfloat64_t fractional = svsub_f64_z (is_real, x, integral);
  return svcreate2 (fractional, integral);
}

#endif
