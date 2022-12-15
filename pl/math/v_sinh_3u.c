/*
 * Double-precision vector sinh(x) function.
 * Copyright (c) 2022, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "v_math.h"
#include "mathlib.h"
#include "pl_sig.h"
#include "pl_test.h"

#define AbsMask 0x7fffffffffffffff
#define Half 0x3fe0000000000000
#define OFlowBound                                                             \
  0x40862e42fefa39f0 /* 0x1.62e42fefa39fp+9, above which using expm1 results   \
			in NaN.  */

#if V_SUPPORTED

/* Approximation for vector double-precision sinh(x) using expm1.
   sinh(x) = (exp(x) - exp(-x)) / 2.
   The greatest observed error is 2.57 ULP:
   sinh(0x1.9fb1d49d1d58bp-2) got 0x1.ab34e59d678dcp-2
			     want 0x1.ab34e59d678d9p-2.  */
VPCS_ATTR v_f64_t V_NAME (sinh) (v_f64_t x)
{
  v_u64_t ix = v_as_u64_f64 (x);
  v_u64_t iax = ix & AbsMask;
  v_f64_t ax = v_as_f64_u64 (iax);
  v_u64_t sign = ix & ~AbsMask;
  v_f64_t halfsign = v_as_f64_u64 (sign | Half);

  v_u64_t special = v_cond_u64 (iax >= OFlowBound);
  /* Fall back to the scalar variant for all lanes if any of them should trigger
     an exception.  */
  if (unlikely (v_any_u64 (special)))
    return v_call_f64 (sinh, x, x, v_u64 (-1));

  /* Up to the point that expm1 overflows, we can use it to calculate sinh
     using a slight rearrangement of the definition of asinh. This allows us to
     retain acceptable accuracy for very small inputs.  */
  v_f64_t t = V_NAME (expm1) (ax);
  return (t + t / (t + 1)) * halfsign;
}
VPCS_ALIAS

PL_SIG (V, D, 1, sinh, -10.0, 10.0)
PL_TEST_ULP (V_NAME (sinh), 2.08)
PL_TEST_EXPECT_FENV (V_NAME (sinh), WANT_ERRNO)
#endif
