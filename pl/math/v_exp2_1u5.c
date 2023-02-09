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

#define C(i) v_f64 (__v_exp2_data.poly[i])

#define BigBound 0x4080000000000000  /* 512.0 = 0x1p+9.  */
#define TinyBound 0x3c90000000000000 /* 0x1p-54.  */

/* Call scalar exp2 as a fallback.  */
VPCS_ATTR
static NOINLINE float64x2_t
specialcase (float64x2_t x)
{
  return v_call_f64 (exp2, x, x, v_u64 (-1));
}

/* Fast implementation of vector exp2.
   Maximum observed error is 1.00 ulps:
   v_exp2(-0x1.8c68c2231567ap-2)
	 got 0x1.8780e9885f8e3p-1
	want 0x1.8780e9885f8e2p-1.  */
VPCS_ATTR float64x2_t V_NAME_D1 (exp2) (float64x2_t x)
{
  uint64x2_t abstop = v_as_u64_f64 (x) & 0x7ff0000000000000;

  /* abstop - 0x1p-54 >= 512.0 - 0x1p-54.  */
  uint64x2_t uoflow = abstop - TinyBound >= BigBound - TinyBound;

  /* When any special cases (underflow, overflow, large x) are possible,
     fall back to scalar.  */
  if (unlikely (v_any_u64 (uoflow)))
    {
      return specialcase (x);
    }

  /* exp2(x) = 2^(k/N) * 2^r, with 2^r in [2^(-1/2N),2^(1/2N)].  */
  /* x = k/N + r, with int k and r in [-1/2N, 1/2N].  */
  float64x2_t kd = x + __v_exp2_data.shift;
  uint64x2_t ki = v_as_u64_f64 (kd); /* k.  */
  kd -= __v_exp2_data.shift;	  /* k/N for int k.  */
  float64x2_t r = x - kd;
  float64x2_t r2 = r * r;

  /* 2^(k/N) ~= scale.  */
  /* n % N <=> n & (N-1) since N is a power of 2.  */
  uint64x2_t idx = ki & (N - 1);
  uint64x2_t tab_sbits = v_lookup_u64 (__v_exp2_data.sbits, idx);
  uint64x2_t top = ki << (52 - V_EXP2_TABLE_BITS);
  /* This is only a valid scale when -1023*N < k < 1024*N.  */
  uint64x2_t sbits = tab_sbits + top;

  /* exp2(x) = 2^(k/N) * 2^r ~= scale + scale * (2^r - 1).  */
  /* Use offset version of Estrin wrapper to evaluate from C1 onwards.
     We avoid forming r2*r2 to avoid overflow.  */
  float64x2_t p1234 = ESTRIN_3_ (r, r2, C, 1);
  float64x2_t p0 = r * v_f64 (__v_exp2_data.poly[0]);
  float64x2_t tmp = v_fma_f64 (r2, p1234, p0);

  float64x2_t scale = v_as_f64_u64 (sbits);
  /* Note: tmp == 0 or |tmp| > 2^-65 and scale > 2^-928, so there
     is no spurious underflow here even without fma.  */
  return v_fma_f64 (scale, tmp, scale);
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
