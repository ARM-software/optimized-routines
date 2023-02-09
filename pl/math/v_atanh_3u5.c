/*
 * Double-precision vector atanh(x) function.
 *
 * Copyright (c) 2022-2023, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "v_math.h"
#include "pairwise_horner.h"
#include "pl_sig.h"
#include "pl_test.h"

#define WANT_V_LOG1P_K0_SHORTCUT 0
#include "v_log1p_inline.h"

#define AbsMask 0x7fffffffffffffff
#define Half 0x3fe0000000000000
#define One 0x3ff0000000000000

VPCS_ATTR
NOINLINE static float64x2_t
specialcase (float64x2_t x, float64x2_t y, uint64x2_t special)
{
  return v_call_f64 (atanh, x, y, special);
}

/* Approximation for vector double-precision atanh(x) using modified log1p.
   The greatest observed error is 3.31 ULP:
   __v_atanh(0x1.ffae6288b601p-6) got 0x1.ffd8ff31b5019p-6
				 want 0x1.ffd8ff31b501cp-6.  */
VPCS_ATTR
float64x2_t V_NAME_D1 (atanh) (float64x2_t x)
{
  uint64x2_t ix = v_as_u64_f64 (x);
  uint64x2_t sign = ix & ~AbsMask;
  uint64x2_t ia = ix & AbsMask;
  uint64x2_t special = ia >= One;
  float64x2_t halfsign = v_as_f64_u64 (sign | Half);

  /* Mask special lanes with 0 to prevent spurious underflow.  */
  float64x2_t ax = vbslq_f64 (special, v_f64 (0), v_as_f64_u64 (ia));
  float64x2_t y = halfsign * log1p_inline ((2 * ax) / (1 - ax));

  if (unlikely (v_any_u64 (special)))
    return specialcase (x, y, special);
  return y;
}

PL_SIG (V, D, 1, atanh, -1.0, 1.0)
PL_TEST_EXPECT_FENV_ALWAYS (V_NAME_D1 (atanh))
PL_TEST_ULP (V_NAME_D1 (atanh), 3.32)
PL_TEST_INTERVAL_C (V_NAME_D1 (atanh), 0, 0x1p-23, 10000, 0)
PL_TEST_INTERVAL_C (V_NAME_D1 (atanh), -0, -0x1p-23, 10000, 0)
PL_TEST_INTERVAL_C (V_NAME_D1 (atanh), 0x1p-23, 1, 90000, 0)
PL_TEST_INTERVAL_C (V_NAME_D1 (atanh), -0x1p-23, -1, 90000, 0)
PL_TEST_INTERVAL_C (V_NAME_D1 (atanh), 1, inf, 100, 0)
PL_TEST_INTERVAL_C (V_NAME_D1 (atanh), -1, -inf, 100, 0)
