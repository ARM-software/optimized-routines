/*
 * Double-precision vector log2 function.
 *
 * Copyright (c) 2022-2023, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "v_math.h"
#include "pl_sig.h"
#include "pl_test.h"

#define V_LOG2_TABLE_BITS 7
#define N (1 << V_LOG2_TABLE_BITS)

static const volatile struct
{
  float64x2_t poly[5];
  float64x2_t invln2;
  uint64x2_t min_norm, special_bound, sign_exp_mask;
} data = {
  /* Each coefficient was generated to approximate log(r) for |r| < 0x1.fp-9
     and N = 128, then scaled by log2(e) in extended precision and rounded back
     to double precision.  */
  .poly = { V2 (-0x1.71547652b83p-1), V2 (0x1.ec709dc340953p-2),
	    V2 (-0x1.71547651c8f35p-2), V2 (0x1.2777ebe12dda5p-2),
	    V2 (-0x1.ec738d616fe26p-3) },
  .invln2 = V2 (0x1.71547652b82fep0),
  .min_norm = V2 (0x0010000000000000),	    /* asuint64(0x1p-1022).  */
  .special_bound = V2 (0x7fe0000000000000), /* asuint64(inf) - min_norm.  */
  .sign_exp_mask = V2 (0xfff0000000000000),
};

#define Off v_u64 (0x3fe6900900000000)
#define IndexMask v_u64 (N - 1)

#define T(s, i) __v_log2_data.s[i]
#define C(i) data.poly[i]

struct entry
{
  float64x2_t invc;
  float64x2_t log2c;
};

static inline struct entry
lookup (uint64x2_t i)
{
  struct entry e;
  e.invc[0] = T (invc, i[0]);
  e.log2c[0] = T (log2c, i[0]);
  e.invc[1] = T (invc, i[1]);
  e.log2c[1] = T (log2c, i[1]);
  return e;
}

static float64x2_t VPCS_ATTR NOINLINE
special_case (float64x2_t x, float64x2_t y, uint64x2_t special)
{
  return v_call_f64 (log2, x, y, special);
}

/* Double-precision vector log2 routine. Implements the same algorithm as
   vector log10, with coefficients and table entries scaled in extended
   precision. The maximum observed error is 2.58 ULP:
   _ZGVnN2v_log2(0x1.0b556b093869bp+0) got 0x1.fffb34198d9dap-5
				      want 0x1.fffb34198d9ddp-5.  */
float64x2_t VPCS_ATTR V_NAME_D1 (log2) (float64x2_t x)
{
  uint64x2_t ix = vreinterpretq_u64_f64 (x);
  uint64x2_t special
      = vcgeq_u64 (vsubq_u64 (ix, data.min_norm), data.special_bound);

  /* x = 2^k z; where z is in range [Off,2*Off) and exact.
     The range is split into N subintervals.
     The ith subinterval contains z and c is near its center.  */
  uint64x2_t tmp = vsubq_u64 (ix, Off);
  uint64x2_t i
      = vandq_u64 (vshrq_n_u64 (tmp, 52 - V_LOG2_TABLE_BITS), IndexMask);
  int64x2_t k = vshrq_n_s64 (vreinterpretq_s64_u64 (tmp), 52);
  uint64x2_t iz = vsubq_u64 (ix, vandq_u64 (tmp, data.sign_exp_mask));
  float64x2_t z = vreinterpretq_f64_u64 (iz);

  struct entry e = lookup (i);

  /* log2(x) = log1p(z/c-1)/log(2) + log2(c) + k.  */

  float64x2_t r = vfmaq_f64 (v_f64 (-1.0), z, e.invc);
  float64x2_t kd = vcvtq_f64_s64 (k);
  float64x2_t w = vfmaq_f64 (e.log2c, r, data.invln2);

  float64x2_t r2 = vmulq_f64 (r, r);
  float64x2_t p_23 = vfmaq_f64 (C (2), C (3), r);
  float64x2_t p_01 = vfmaq_f64 (C (0), C (1), r);
  float64x2_t y;
  y = vfmaq_f64 (p_23, C (4), r2);
  y = vfmaq_f64 (p_01, r2, y);
  y = vfmaq_f64 (vaddq_f64 (kd, w), r2, y);

  if (unlikely (v_any_u64 (special)))
    return special_case (x, y, special);
  return y;
}

PL_SIG (V, D, 1, log2, 0.01, 11.1)
PL_TEST_ULP (V_NAME_D1 (log2), 2.09)
PL_TEST_EXPECT_FENV_ALWAYS (V_NAME_D1 (log2))
PL_TEST_INTERVAL (V_NAME_D1 (log2), -0.0, -0x1p126, 100)
PL_TEST_INTERVAL (V_NAME_D1 (log2), 0x1p-149, 0x1p-126, 4000)
PL_TEST_INTERVAL (V_NAME_D1 (log2), 0x1p-126, 0x1p-23, 50000)
PL_TEST_INTERVAL (V_NAME_D1 (log2), 0x1p-23, 1.0, 50000)
PL_TEST_INTERVAL (V_NAME_D1 (log2), 1.0, 100, 50000)
PL_TEST_INTERVAL (V_NAME_D1 (log2), 100, inf, 50000)
