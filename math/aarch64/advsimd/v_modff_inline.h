/*
 * Inline helpers for AdvSIMD modff.
 *
 * Copyright (c) 2025, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#ifndef V_MODFF_INLINE_H
#define V_MODFF_INLINE_H

#include "v_math.h"

static inline float32x4x2_t
v_modff_inline (float32x4_t x)
{
  float32x4_t integral = vrndq_f32 (x);
  uint32x4_t frac_bits = vreinterpretq_u32_f32 (vsubq_f32 (x, integral));
  uint32x4_t is_integer = vceqq_f32 (x, integral);
  float32x4_t fractional
      = vreinterpretq_f32_u32 (vbicq_u32 (frac_bits, is_integer));
  return (float32x4x2_t){ .val = { fractional, integral } };
}

#endif
