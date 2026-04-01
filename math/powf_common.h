/*
 * Single-precision pow helper functions.
 *
 * Copyright (c) 2026, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */
#ifndef POWF_COMMON_H
#define POWF_COMMON_H

#include <stdint.h>
#include "math_config.h"

/* Returns 0 if not int, 1 if odd int, 2 if even int.  The argument is
   the bit representation of a non-zero finite floating-point value.  */
static inline int
checkint (uint32_t iy)
{
  int e = iy >> 23 & 0xff;
  if (e < 0x7f)
    return 0;
  if (e > 0x7f + 23)
    return 2;
  if (iy & ((1 << (0x7f + 23 - e)) - 1))
    return 0;
  if (iy & (1 << (0x7f + 23 - e)))
    return 1;
  return 2;
}

/* Returns 1 if input is the bit representation of 0, infinity or nan.  */
static inline int
zeroinfnan (uint32_t ix)
{
  return 2 * ix - 1 >= 2u * 0x7f800000 - 1;
}

#endif
