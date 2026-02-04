/*
 * Double-precision vector e^x function.
 *
 * Copyright (c) 2019-2026, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "mathlib.h"
#include "v_math.h"
#include "test_defs.h"
#include "test_sig.h"
#include "v_exp_special_case_inline.h"

/* Value of |x| above which scale overflows without special treatment.  */
#define SpecialBound 0x1.6232bdd76683cp+9 /* ln(2^1022) ~ 708.40.  */

/* Value of n above which scale overflows even with special treatment.  */
#define ScaleBound 163840.0 /* 1280.0 * N.  */

static const struct data
{
  struct v_exp_special_data special_data;
  float64x2_t inv_ln2, shift;
  float64x2_t special_bound, scale_thresh, c0;
  double ln2_hi, ln2_lo;
  double c1, c2;
} data = {
  .special_data = V_EXP_SPECIAL_DATA,
  /* maxerr: 1.88 +0.5 ulp
     rel error: 1.4337*2^-53
     abs error: 1.4299*2^-53 in [ -ln2/256, ln2/256 ].  */
  .c0 = V2 (0x1.ffffffffffd43p-2),
  .c1 = 0x1.55555c75adbb2p-3,
  .c2 = 0x1.55555da646206p-5,
  .scale_thresh = V2 (ScaleBound),
  /* Implementation triggers special case handling as soon as the scale
     overflows, which is earlier than exp's overflow bound
     `ln(DBL_MAX) ≈ 709.78` or underflow bound
     `ln(DBL_TRUE_MIN) ≈ -744.44`.
     The absolute comparison catches all these cases efficiently, although
     there is a small window where special cases are triggered
     unnecessarily.  */
  .special_bound = V2 (SpecialBound),
  .inv_ln2 = V2 (0x1.71547652b82fep7), /* N/ln2.  */
  .ln2_hi = 0x1.62e42fefa39efp-8,      /* ln2/N.  */
  .ln2_lo = 0x1.abc9e3b39803f3p-63,
  .shift = V2 (0x1.8p+52),
};

#define N (1 << V_EXP_TABLE_BITS)
#define IndexMask (N - 1)

static inline uint64x2_t
lookup_sbits (uint64x2_t i)
{
  return (uint64x2_t){ __v_exp_data[i[0] & IndexMask],
		       __v_exp_data[i[1] & IndexMask] };
}

/* Fast vector implementation of exp.
   Maximum measured error is 1.9 + 0.5ulp.
   _ZGVnN2v_expm1(0x1.63b3d5efc2e51p-2) got 0x1.a94d602bf433ep-2
				       want 0x1.a94d602bf433cp-2.  */
float64x2_t VPCS_ATTR V_NAME_D1 (exp) (float64x2_t x)
{
  const struct data *d = ptr_barrier (&data);

  /* n = round(x/(ln2/N)).  */
  float64x2_t z = vfmaq_f64 (d->shift, x, d->inv_ln2);
  uint64x2_t u = vreinterpretq_u64_f64 (z);
  float64x2_t n = vsubq_f64 (z, d->shift);

  /* r = x - n*ln2/N.  */
  float64x2_t ln2_hi_lo = vld1q_f64 (&d->ln2_hi);
  float64x2_t r = x;
  r = vfmsq_laneq_f64 (r, n, ln2_hi_lo, 0);
  r = vfmsq_laneq_f64 (r, n, ln2_hi_lo, 1);

  uint64x2_t e = vshlq_n_u64 (u, 52 - V_EXP_TABLE_BITS);

  /* poly = exp(r) - 1 ~= r + C0 r^2 + C1 r^3 + C2 r^4.  */
  float64x2_t c12 = vld1q_f64 (&d->c1);
  float64x2_t r2 = vmulq_f64 (r, r);
  float64x2_t poly = vfmaq_laneq_f64 (d->c0, r, c12, 0);
  poly = vfmaq_laneq_f64 (poly, r2, c12, 1);
  poly = vfmaq_f64 (r, poly, r2);

  /* scale = 2^(n/N).  */
  u = lookup_sbits (u);
  float64x2_t scale = vreinterpretq_f64_u64 (vaddq_u64 (u, e));

  uint64x2_t cmp = vcagtq_f64 (x, d->special_bound);

  if (unlikely (v_any_u64 (cmp)))
    return exp_special (poly, n, scale, d->scale_thresh, &d->special_data);

  return vfmaq_f64 (scale, poly, scale);
}

TEST_SIG (V, D, 1, exp, -9.9, 9.9)
TEST_ULP (V_NAME_D1 (exp), 1.9)
TEST_INTERVAL (V_NAME_D1 (exp), 0, 0xffff000000000000, 10000)
TEST_SYM_INTERVAL (V_NAME_D1 (exp), 0, 0x1p-23, 10000)
TEST_SYM_INTERVAL (V_NAME_D1 (exp), 0x1p-23, SpecialBound, 100000)
TEST_SYM_INTERVAL (V_NAME_D1 (exp), SpecialBound, ScaleBound, 10000)
TEST_SYM_INTERVAL (V_NAME_D1 (exp), SpecialBound, inf, 10000)
