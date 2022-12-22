/*
 * Double-precision vector atanh(x) function.
 *
 * Copyright (c) 2022, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "v_math.h"
#include "pairwise_horner.h"
#include "pl_sig.h"
#include "pl_test.h"

#if V_SUPPORTED

#define Ln2Hi v_f64 (0x1.62e42fefa3800p-1)
#define Ln2Lo v_f64 (0x1.ef35793c76730p-45)
#define HfRt2Top 0x3fe6a09e00000000 /* top32(asuint64(sqrt(2)/2)) << 32.  */
#define OneMHfRt2Top                                                           \
  0x00095f6200000000 /* (top32(asuint64(1)) - top32(asuint64(sqrt(2)/2)))      \
			<< 32.  */
#define OneTop12 0x3ff
#define BottomMask 0xffffffff

#define AbsMask 0x7fffffffffffffff
#define Half 0x3fe0000000000000
#define One 0x3ff0000000000000

#define C(i) v_f64 (__log1p_data.coeffs[i])

static inline v_f64_t
log1p_inline (v_f64_t x)
{
  /* Helper for calculating log(1 + x) using order-18 polynomial on a reduced
     interval. Copied from v_log1p_2u5.c, with the following modifications:
     - No special-case handling.
     - Pairwise Horner instead of Estrin for improved accuracy.
     - Slightly different recombination to reuse f2.
     See original source for details of the algorithm.  */
  v_f64_t m = x + 1;
  v_u64_t mi = v_as_u64_f64 (m);

  /* Decompose x + 1 into (f + 1) * 2^k, with k chosen such that f is in
     [sqrt(2)/2, sqrt(2)].  */
  v_u64_t u = mi + OneMHfRt2Top;
  v_s64_t ki = v_as_s64_u64 (u >> 52) - OneTop12;
  v_f64_t k = v_to_f64_s64 (ki);
  v_u64_t utop = (u & 0x000fffff00000000) + HfRt2Top;
  v_u64_t u_red = utop | (mi & BottomMask);
  v_f64_t f = v_as_f64_u64 (u_red) - 1;

  /* Correction term for round-off in f.  */
  v_f64_t cm = (x - (m - 1)) / m;

  /* Approximate log1p(f) with polynomial.  */
  v_f64_t f2 = f * f;
  v_f64_t p = PAIRWISE_HORNER_18 (f, f2, C);

  /* Recombine log1p(x) = k*log2 + log1p(f) + c/m.  */
  v_f64_t ylo = v_fma_f64 (k, Ln2Lo, cm);
  v_f64_t yhi = v_fma_f64 (k, Ln2Hi, f);
  v_f64_t y = v_fma_f64 (f2, p, ylo + yhi);
  return y;
}

VPCS_ATTR
NOINLINE static v_f64_t
specialcase (v_f64_t x, v_f64_t y, v_u64_t special)
{
  return v_call_f64 (atanh, x, y, special);
}

/* Approximation for vector double-precision atanh(x) using modified log1p.
   The greatest observed error is 3.31 ULP:
   __v_atanh(0x1.ffae6288b601p-6) got 0x1.ffd8ff31b5019p-6
				 want 0x1.ffd8ff31b501cp-6.  */
VPCS_ATTR
v_f64_t V_NAME (atanh) (v_f64_t x)
{
  v_u64_t ix = v_as_u64_f64 (x);
  v_u64_t sign = ix & ~AbsMask;
  v_u64_t ia = ix & AbsMask;
  v_u64_t special = v_cond_u64 (ia >= One);
  v_f64_t halfsign = v_as_f64_u64 (sign | Half);

  /* Mask special lanes with 0 to prevent spurious underflow.  */
  v_f64_t ax = v_sel_f64 (special, v_f64 (0), v_as_f64_u64 (ia));
  v_f64_t y = halfsign * log1p_inline ((2 * ax) / (1 - ax));

  if (unlikely (v_any_u64 (special)))
    return specialcase (x, y, special);
  return y;
}
VPCS_ALIAS

PL_SIG (V, D, 1, atanh, -1.0, 1.0)
PL_TEST_EXPECT_FENV_ALWAYS (V_NAME (atanh))
PL_TEST_ULP (V_NAME (atanh), 3.32)
PL_TEST_INTERVAL_C (V_NAME (atanh), 0, 0x1p-23, 10000, 0)
PL_TEST_INTERVAL_C (V_NAME (atanh), -0, -0x1p-23, 10000, 0)
PL_TEST_INTERVAL_C (V_NAME (atanh), 0x1p-23, 1, 90000, 0)
PL_TEST_INTERVAL_C (V_NAME (atanh), -0x1p-23, -1, 90000, 0)
PL_TEST_INTERVAL_C (V_NAME (atanh), 1, inf, 100, 0)
PL_TEST_INTERVAL_C (V_NAME (atanh), -1, -inf, 100, 0)
#endif
