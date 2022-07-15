/*
 * Single-precision vector asinh(x) function.
 * Copyright (c) 2022, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "v_math.h"
#include "include/mathlib.h"

#if V_SUPPORTED

#define SignMask v_u32 (0x80000000)
#define One v_f32 (1.0f)
#define Ln2 v_f32 (0x1.62e43p-1f)
#define SpecialBound v_u32 (0x5f800000) /* asuint(0x1p64).  */

static inline v_f32_t
handle_special (v_f32_t ax)
{
  return V_NAME (log1pf) (ax) + Ln2;
}

/* Single-precision implementation of vector asinh(x), using vector log1p.
   Worst-case error is 2.66 ULP, at roughly +/-0.25:
   __v_asinhf(0x1.01b04p-2) got 0x1.fe163ep-3 want 0x1.fe1638p-3.  */
VPCS_ATTR v_f32_t V_NAME (asinhf) (v_f32_t x)
{
  v_f32_t ax = v_abs_f32 (x);
  v_u32_t special = v_cond_u32 (v_as_u32_f32 (ax) >= SpecialBound);
  v_u32_t sign = v_as_u32_f32 (x) & SignMask;

  /* asinh(x) = log(x + sqrt(x * x + 1)).
     For positive x, asinh(x) = log1p(x + x * x / (1 + sqrt(x * x + 1))).  */
  v_f32_t d = One + v_sqrt_f32 (ax * ax + One);
  v_f32_t y = V_NAME (log1pf) (ax + ax * ax / d);

  if (unlikely (v_any_u32 (special)))
    {
      /* If |x| is too large, we cannot square it at low cost without overflow.
	 At very large x, asinh(x) ~= log(2x) and log(x) ~= log1p(x), so we
	 calculate asinh(x) as log1p(x) + log(2).  */
      v_f32_t y_large = V_NAME (log1pf) (ax) + Ln2;
      return v_as_f32_u32 (sign
			   | v_as_u32_f32 (v_sel_f32 (special, y_large, y)));
    }

  return v_as_f32_u32 (sign | v_as_u32_f32 (y));
}
VPCS_ALIAS

#endif
