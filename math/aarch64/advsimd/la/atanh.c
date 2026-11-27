/*
 * Double-precision vector atanh(x) function.
 *
 * Copyright (c) 2022-2026, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "v_math.h"
#include "test_sig.h"
#include "test_defs.h"

#define WANT_V_LOG1P_K0_SHORTCUT 0
#include "v_log1p_inline.h"

const static struct data
{
  struct v_log1p_data log1p_consts;
  float64x2_t one;
  uint64x2_t sign_mask;
} data = { .log1p_consts = V_LOG1P_CONSTANTS_TABLE,
	   .one = V2 (1.0),
	   .sign_mask = V2 (0x8000000000000000) };

static float64x2_t VPCS_ATTR NOINLINE
special_case (float64x2_t ax, float64x2_t halfsign, float64x2_t y,
	      uint64x2_t special, const struct data *d)
{
  y = log1p_inline (y, &d->log1p_consts);
  /* Copy the sign bit from the input to inf.  */
  uint64x2_t is_one = vceqq_f64 (ax, d->one);
  float64x2_t signed_inf
      = vbslq_f64 (d->sign_mask, halfsign, v_f64 (INFINITY));
  /* Here we check for all the rest of the cases where |x| > 1 will return a
     NaN, including if x = NaN.  */
  float64x2_t res = vbslq_f64 (special, v_f64 (NAN), vmulq_f64 (halfsign, y));

  return vbslq_f64 (is_one, signed_inf, res);
}

/* Approximation for vector double-precision atanh(x) using modified log1p.
   The greatest observed error is 2.81 + 0.5 ULP:
   _ZGVnN2v_atanh(0x1.ffae6288b601p-6) got 0x1.ffd8ff31b5019p-6
				      want 0x1.ffd8ff31b501cp-6.  */
VPCS_ATTR float64x2_t V_NAME_D1 (atanh) (float64x2_t x)
{
  const struct data *d = ptr_barrier (&data);

  float64x2_t halfsign = vbslq_f64 (d->sign_mask, x, v_f64 (0.5));
  float64x2_t ax = vabsq_f64 (x);
  uint64x2_t special = vcageq_f64 (x, d->one);

  float64x2_t y;
  y = vaddq_f64 (ax, ax);
  y = vdivq_f64 (y, vsubq_f64 (d->one, ax));

  if (unlikely (v_any_u64 (special)))
    return special_case (ax, halfsign, y, special, d);

  y = log1p_inline (y, &d->log1p_consts);
  return vmulq_f64 (y, halfsign);
}

TEST_SIG (V, D, 1, atanh, -1.0, 1.0)
TEST_ULP (V_NAME_D1 (atanh), 2.81)
TEST_SYM_INTERVAL (V_NAME_D1 (atanh), 0, 0x1p-23, 10000)
TEST_SYM_INTERVAL (V_NAME_D1 (atanh), 0x1p-23, 1, 90000)
TEST_SYM_INTERVAL (V_NAME_D1 (atanh), 1, inf, 100)
