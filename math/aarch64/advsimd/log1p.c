/*
 * Double-precision vector log(1+x) function.
 *
 * Copyright (c) 2022-2025, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "v_math.h"
#include "test_sig.h"
#include "test_defs.h"

#define WANT_V_LOG1P_K0_SHORTCUT 0
#include "v_log1p_inline.h"

const static struct data
{
  struct v_log1p_data d;
  float64x2_t nan, pinf, minf;
} data = {
  .d = V_LOG1P_CONSTANTS_TABLE,
  .nan = V2 (NAN),
  .pinf = V2 (INFINITY),
  .minf = V2 (-INFINITY),
};

static inline float64x2_t
special_case (float64x2_t x, uint64x2_t cmp, const struct data *d)
{
  float64x2_t y = log1p_inline (x, ptr_barrier (&d->d));
  y = vbslq_f64 (cmp, d->nan, y);
  uint64x2_t ret_pinf = vceqq_f64 (x, d->pinf);
  uint64x2_t ret_minf = vceqq_f64 (x, v_f64 (-1.0));

  y = vbslq_f64 (ret_pinf, d->pinf, y);
  return vbslq_f64 (ret_minf, d->minf, y);
}

/* Vector log1p approximation using polynomial on reduced interval. Routine is
   a modification of the algorithm used in scalar log1p, with no shortcut for
   k=0 and no narrowing for f and k.
   Maximum observed error is 1.95 + 0.5 ULP
   _ZGVnN2v_log1p(0x1.658f7035c4014p+11) got 0x1.fd61d0727429dp+2
					want 0x1.fd61d0727429fp+2 .  */
float64x2_t VPCS_ATTR NOINLINE V_NAME_D1 (log1p) (float64x2_t x)
{
  const struct data *d = ptr_barrier (&data);

  /* Use signed integers here to ensure that negative numbers between 0 and -1
    don't make this expression true.  */
  uint64x2_t is_infnan
      = vcgeq_s64 (vreinterpretq_s64_f64 (x), vreinterpretq_s64_f64 (d->pinf));
  /* The OR-NOT is needed to catch -NaN.  */
  uint64x2_t special_cases = vornq_u64 (is_infnan, vcgtq_f64 (x, v_f64 (-1)));

  if (unlikely (v_any_u64 (special_cases)))
    return special_case (x, special_cases, d);

  return log1p_inline (x, &d->d);
}

TEST_SIG (V, D, 1, log1p, -0.9, 10.0)
TEST_ULP (V_NAME_D1 (log1p), 1.95)
TEST_SYM_INTERVAL (V_NAME_D1 (log1p), 0.0, 0x1p-23, 50000)
TEST_SYM_INTERVAL (V_NAME_D1 (log1p), 0x1p-23, 0.001, 50000)
TEST_SYM_INTERVAL (V_NAME_D1 (log1p), 0.001, 1.0, 50000)
TEST_INTERVAL (V_NAME_D1 (log1p), 1, inf, 40000)
TEST_INTERVAL (V_NAME_D1 (log1p), -1.0, -inf, 500)
