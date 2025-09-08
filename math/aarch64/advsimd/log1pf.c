/*
 * Single-precision vector log(1+x) function.
 *
 * Copyright (c) 2022-2025, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "v_math.h"
#include "test_sig.h"
#include "test_defs.h"
#include "v_log1pf_inline.h"

const static struct v_log1pf_data data = V_LOG1PF_CONSTANTS_TABLE;

static float32x4_t NOINLINE VPCS_ATTR
special_case (float32x4_t x, uint32x4_t cmp)
{
  return v_call_f32 (log1pf, x, log1pf_inline (x, ptr_barrier (&data)), cmp);
}

/* Vector log1pf approximation using polynomial on reduced interval. Worst-case
   error is 1.63 ULP:
   _ZGVnN4v_log1pf(0x1.216d12p-2) got 0x1.fdcb12p-3
				 want 0x1.fdcb16p-3.  */
float32x4_t VPCS_ATTR NOINLINE V_NAME_F1 (log1p) (float32x4_t x)
{
  uint32x4_t special_cases = vornq_u32 (vcleq_f32 (x, v_f32 (-1)),
					vcaleq_f32 (x, v_f32 (0x1p127f)));

  if (unlikely (v_any_u32 (special_cases)))
    return special_case (x, special_cases);

  return log1pf_inline (x, ptr_barrier (&data));
}

HALF_WIDTH_ALIAS_F1 (log1p)

TEST_SIG (V, F, 1, log1p, -0.9, 10.0)
TEST_ULP (V_NAME_F1 (log1p), 1.20)
TEST_SYM_INTERVAL (V_NAME_F1 (log1p), 0.0, 0x1p-23, 30000)
TEST_SYM_INTERVAL (V_NAME_F1 (log1p), 0x1p-23, 1, 50000)
TEST_INTERVAL (V_NAME_F1 (log1p), 1, inf, 50000)
TEST_INTERVAL (V_NAME_F1 (log1p), -1.0, -inf, 1000)
