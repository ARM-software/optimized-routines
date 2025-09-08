/*
 * Double-precision vector hypot(x) function.
 *
 * Copyright (c) 2023-2025, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "v_math.h"
#include "test_sig.h"
#include "test_defs.h"

static const struct data
{
  uint64x2_t tiny_bound;
  uint32x4_t thres;
} data = {
  .tiny_bound = V2 (0x0360000000000000), /* asuint (0x1p-969).  */
  .thres = V4 (0x7c900000),		 /* asuint (inf) - tiny_bound.  */
};

static float64x2_t VPCS_ATTR NOINLINE
special_case (float64x2_t x, float64x2_t y, float64x2_t sqsum,
	      uint32x2_t special)
{
  return v_call2_f64 (hypot, x, y, vsqrtq_f64 (sqsum), vmovl_u32 (special));
}

/* Vector implementation of double-precision hypot.
   Maximum error observed is 1.21 ULP:
   _ZGVnN2vv_hypot (0x1.6a1b193ff85b5p-204, 0x1.bc50676c2a447p-222)
    got 0x1.6a1b19400964ep-204
   want 0x1.6a1b19400964dp-204.  */
float64x2_t VPCS_ATTR V_NAME_D2 (hypot) (float64x2_t x, float64x2_t y)
{
  const struct data *d = ptr_barrier (&data);

  float64x2_t sqsum = vfmaq_f64 (vmulq_f64 (x, x), y, y);

  uint32x2_t special
      = vcge_u32 (vsubhn_u64 (vreinterpretq_u64_f64 (sqsum), d->tiny_bound),
		  vget_low_u32 (d->thres));

  if (unlikely (v_any_u32h (special)))
    return special_case (x, y, sqsum, special);

  return vsqrtq_f64 (sqsum);
}

TEST_SIG (V, D, 2, hypot, -10.0, 10.0)
TEST_ULP (V_NAME_D2 (hypot), 0.71)
TEST_INTERVAL2 (V_NAME_D2 (hypot), 0, inf, 0, inf, 10000)
TEST_INTERVAL2 (V_NAME_D2 (hypot), 0, inf, -0, -inf, 10000)
TEST_INTERVAL2 (V_NAME_D2 (hypot), -0, -inf, 0, inf, 10000)
TEST_INTERVAL2 (V_NAME_D2 (hypot), -0, -inf, -0, -inf, 10000)
