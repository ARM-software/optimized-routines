/*
 * Inline helpers for AdvSIMD modf.
 *
 * Copyright (c) 2025, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#ifndef V_MODF_INLINE_H
#define V_MODF_INLINE_H

#include "v_math.h"

static inline float64x2x2_t
v_modf_inline (float64x2_t x)
{
  float64x2_t integral = vrndq_f64 (x);
  uint64x2_t frac_bits = vreinterpretq_u64_f64 (vsubq_f64 (x, integral));
  uint64x2_t is_integer = vceqq_f64 (x, integral);
  float64x2_t fractional
      = vreinterpretq_f64_u64 (vbicq_u64 (frac_bits, is_integer));
  return (float64x2x2_t){ .val = { fractional, integral } };
}

#endif
