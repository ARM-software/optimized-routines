/*
 * Double-precision vector 2^x function.
 *
 * Copyright (c) 2023, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "v_math.h"
#include "estrin.h"
#include "pl_sig.h"
#include "pl_test.h"

#define N (1 << V_EXP2_TABLE_BITS)

struct v_exp2_local_data
{
  float64x2_t poly[V_EXP2_POLY_ORDER];
  float64x2_t shift;
};

static const volatile struct v_exp2_local_data data = {
  /* Shift, coefficients and sbits entries taken from exp_data.h for N == 128
     and EXP2_POLY_ORDER == 5.  */
  .shift = V2 (0x1.8p52 / N),
  .poly = {V2 (0x1.62e42fefa39efp-1), V2 (0x1.ebfbdff82c424p-3),
	   V2 (0x1.c6b08d70cf4b5p-5), V2 (0x1.3b2abd24650ccp-7),
	   V2 (0x1.5d7e09b4e3a84p-10)},
};

#define AllMask v_u64 (0xffffffffffffffff)
#define ExponentMask v_u64 (0x7ff0000000000000)
#define TinyBound 0x3c90000000000000	 /* asuint64(0x1p-54).  */
#define BigBound 0x4080000000000000	 /* asuint64(512.0).  */
#define Thres v_u64 (0x03f0000000000000) /* BigBound - TinyBound.  */
#define C(i) data.poly[i]

static inline uint64x2_t
lookup_sbits (uint64x2_t i)
{
  return (uint64x2_t){__v_exp2_data[i[0]], __v_exp2_data[i[1]]};
}

/* Call scalar exp2 as a fallback.  */
static float64x2_t VPCS_ATTR NOINLINE
special_case (float64x2_t x)
{
  return v_call_f64 (exp2, x, x, AllMask);
}

/* Fast implementation of vector exp2.
   Maximum observed error is 1.00 ulps:
   v_exp2(-0x1.8c68c2231567ap-2)
	 got 0x1.8780e9885f8e3p-1
	want 0x1.8780e9885f8e2p-1.  */
float64x2_t VPCS_ATTR V_NAME_D1 (exp2) (float64x2_t x)
{
  uint64x2_t abstop = vandq_u64 (vreinterpretq_u64_f64 (x), ExponentMask);

  /* abstop - 0x1p-54 >= 512.0 - 0x1p-54.  */
  uint64x2_t uoflow = vcgeq_u64 (vsubq_u64 (abstop, v_u64 (TinyBound)), Thres);

  /* When any special cases (underflow, overflow, large x) are possible,
     fall back to scalar.  */
  if (unlikely (v_any_u64 (uoflow)))
    return special_case (x);

  /* exp2(x) = 2^(k/N) * 2^r, with 2^r in [2^(-1/2N),2^(1/2N)].  */
  /* x = k/N + r, with int k and r in [-1/2N, 1/2N].  */
  float64x2_t kd = vaddq_f64 (x, data.shift);
  uint64x2_t ki = vreinterpretq_u64_f64 (kd); /* k.  */
  kd = vsubq_f64 (kd, data.shift);	      /* k/N for int k.  */
  float64x2_t r = vsubq_f64 (x, kd);
  float64x2_t r2 = vmulq_f64 (r, r);

  /* 2^(k/N) ~= scale.  */
  uint64x2_t idx = vandq_u64 (ki, v_u64 (N - 1));
  uint64x2_t sbits = lookup_sbits (idx);
  uint64x2_t top = vshlq_n_u64 (ki, 52 - V_EXP2_TABLE_BITS);
  /* This is only a valid scale when -1023*N < k < 1024*N.  */
  sbits = vaddq_u64 (sbits, top);

  /* exp2(x) = 2^(k/N) * 2^r ~= scale + scale * (2^r - 1).  */
  /* Use offset version of Estrin wrapper to evaluate from C1 onwards.
     We avoid forming r2*r2 to avoid overflow.  */
  float64x2_t p = ESTRIN_3_ (r, r2, C, 1);
  float64x2_t p0 = vmulq_f64 (r, C (0));
  float64x2_t poly = vfmaq_f64 (p0, r2, p);

  float64x2_t scale = vreinterpretq_f64_u64 (sbits);
  /* Note: poly == 0 or |poly| > 2^-65 and scale > 2^-928, so there
     is no spurious underflow here even without fma.  */
  return vfmaq_f64 (scale, scale, poly);
}

PL_SIG (V, D, 1, exp2, -9.9, 9.9)
PL_TEST_ULP (V_NAME_D1 (exp2), 0.51)
PL_TEST_EXPECT_FENV_ALWAYS (V_NAME_D1 (exp2))
PL_TEST_INTERVAL (V_NAME_D1 (exp2), 0, TinyBound, 1000)
PL_TEST_INTERVAL (V_NAME_D1 (exp2), TinyBound, BigBound, 100000)
PL_TEST_INTERVAL (V_NAME_D1 (exp2), BigBound, inf, 1000)
PL_TEST_INTERVAL (V_NAME_D1 (exp2), -0, -TinyBound, 1000)
PL_TEST_INTERVAL (V_NAME_D1 (exp2), -TinyBound, -BigBound, 100000)
PL_TEST_INTERVAL (V_NAME_D1 (exp2), -BigBound, -inf, 1000)
