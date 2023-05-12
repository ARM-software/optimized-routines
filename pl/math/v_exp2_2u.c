/*
 * Double-precision vector 2^x function.
 *
 * Copyright (c) 2019-2023, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "v_math.h"
#include "estrin.h"
#include "pl_sig.h"
#include "pl_test.h"

struct v_exp2_data
{
  float64x2_t poly[4];
  float64x2_t shift, uoflow_bound;
};

#define N (1 << V_EXP_TABLE_BITS)
#define IndexMask v_u64 (N - 1)
#define AllMask v_u64 (0xffffffffffffffff)
#define TinyBound 0x2000000000000000 /* asuint64(0x1p-511).  */
#define Thres 0x2080000000000000     /* asuint64(512.0) - TinyBound.  */
#define BigBound 1022.0
#define UOFlowBound 1280.0

static const volatile struct v_exp2_data data
  = {.shift = V2 (0x1.8p52 / N),
     .uoflow_bound = V2 (UOFlowBound),
     /* Coefficients are reproducible using math/tools/exp2.sollya with
	minimisation of the absolute error.  */
     .poly = {V2 (0x1.62e42fefa3686p-1), V2 (0x1.ebfbdff82c241p-3),
	      V2 (0x1.c6b09b16de99ap-5), V2 (0x1.3b2abf5571ad8p-7)}};

#define C(i) data.poly[i]
#define SpecialOffset 0x6000000000000000 /* 0x1p513.  */
/* SpecialBias1 + SpecialBias1 = asuint(1.0).  */
#define SpecialBias1 0x7000000000000000 /* 0x1p769.  */
#define SpecialBias2 0x3010000000000000 /* 0x1p-254.  */

static inline uint64x2_t
lookup_sbits (uint64x2_t i)
{
  return (uint64x2_t){__v_exp_data[i[0]], __v_exp_data[i[1]]};
}

#if WANT_SIMD_EXCEPT

/* Call scalar exp2 as a fallback.  */
static float64x2_t VPCS_ATTR NOINLINE
special_case (float64x2_t x)
{
  return v_call_f64 (exp2, x, x, AllMask);
}

#else

static float64x2_t VPCS_ATTR
special_case (float64x2_t s, float64x2_t y, float64x2_t n)
{
  /* 2^(n/N) may overflow, break it up into s1*s2.  */
  uint64x2_t b = vandq_u64 (vclezq_f64 (n), v_u64 (SpecialOffset));
  float64x2_t s1 = vreinterpretq_f64_u64 (vsubq_u64 (v_u64 (SpecialBias1), b));
  float64x2_t s2 = vreinterpretq_f64_u64 (
    vaddq_u64 (vsubq_u64 (vreinterpretq_u64_f64 (s), v_u64 (SpecialBias2)), b));
  uint64x2_t cmp = vcagtq_f64 (n, data.uoflow_bound);
  float64x2_t r1 = vmulq_f64 (s1, s1);
  float64x2_t r0 = vmulq_f64 (vfmaq_f64 (s2, s2, y), s1);
  return vbslq_f64 (cmp, r1, r0);
}

#endif

/* Fast vector implementation of exp2.
   Maximum measured error is 1.65 ulp.
   _ZGVnN2v_exp2(-0x1.4c264ab5b559bp-6) got 0x1.f8db0d4df721fp-1
				       want 0x1.f8db0d4df721dp-1.  */
VPCS_ATTR
float64x2_t V_NAME_D1 (exp2) (float64x2_t x)
{
  uint64x2_t cmp;
#if WANT_SIMD_EXCEPT
  uint64x2_t ia = vreinterpretq_u64_f64 (vabsq_f64 (x));
  cmp = vcgeq_u64 (vsubq_u64 (ia, v_u64 (TinyBound)), v_u64 (Thres));
  /* If any special case (inf, nan, small and large x) is detected,
     fall back to scalar for all lanes.  */
  if (unlikely (v_any_u64 (cmp)))
    return special_case (x);
#else
  cmp = vcagtq_f64 (x, v_f64 (BigBound));
#endif

  /* n = round(x/N).  */
  float64x2_t z = vaddq_f64 (data.shift, x);
  uint64x2_t u = vreinterpretq_u64_f64 (z);
  float64x2_t n = vsubq_f64 (z, data.shift);

  /* r = x - n/N.  */
  float64x2_t r = vsubq_f64 (x, n);

  /* s = 2^(n/N).  */
  uint64x2_t e = vshlq_n_u64 (u, 52 - V_EXP_TABLE_BITS);
  uint64x2_t i = vandq_u64 (u, IndexMask);
  u = lookup_sbits (i);
  float64x2_t s = vreinterpretq_f64_u64 (vaddq_u64 (u, e));

  /* y ~ exp2(r) - 1.  */
  float64x2_t r2 = vmulq_f64 (r, r);
  float64x2_t y = ESTRIN_3 (r, r2, C);
  y = vmulq_f64 (r, y);

#if !WANT_SIMD_EXCEPT
  if (unlikely (v_any_u64 (cmp)))
    return special_case (s, y, n);
#endif
  return vfmaq_f64 (s, s, y);
}

PL_SIG (V, D, 1, exp2, -9.9, 9.9)
PL_TEST_ULP (V_NAME_D1 (exp2), 1.15)
PL_TEST_EXPECT_FENV (V_NAME_D1 (exp2), WANT_SIMD_EXCEPT)
PL_TEST_INTERVAL (V_NAME_D1 (exp2), 0, TinyBound, 5000)
PL_TEST_INTERVAL (V_NAME_D1 (exp2), TinyBound, BigBound, 10000)
PL_TEST_INTERVAL (V_NAME_D1 (exp2), BigBound, UOFlowBound, 5000)
PL_TEST_INTERVAL (V_NAME_D1 (exp2), UOFlowBound, inf, 10000)
PL_TEST_INTERVAL (V_NAME_D1 (exp2), -0, -TinyBound, 5000)
PL_TEST_INTERVAL (V_NAME_D1 (exp2), -TinyBound, -BigBound, 10000)
PL_TEST_INTERVAL (V_NAME_D1 (exp2), -BigBound, -UOFlowBound, 5000)
PL_TEST_INTERVAL (V_NAME_D1 (exp2), -UOFlowBound, -inf, 10000)
