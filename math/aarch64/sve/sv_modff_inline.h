/*
 * Inline helpers for SVE modff.
 *
 * Copyright (c) 2025, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#ifndef SV_MODFF_INLINE_H
#define SV_MODFF_INLINE_H

#include "sv_math.h"

static inline svfloat32x2_t
sv_modff_inline (svbool_t pg, svfloat32_t x)
{
  svfloat32_t integral = svrintz_x (pg, x);
  svbool_t is_real = svcmpne (pg, x, integral);
  svfloat32_t fractional = svsub_f32_z (is_real, x, integral);
  return svcreate2 (fractional, integral);
}

#endif
