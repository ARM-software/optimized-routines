/*
 * Single-precision vector tan(x) function.
 *
 * Copyright (c) 2021-2022, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "v_math.h"
#include "estrinf.h"

#if V_SUPPORTED

/* Constants.  */
#define NegPio2_1 (v_f32 (-0x1.921fb6p+0f))
#define NegPio2_2 (v_f32 (0x1.777a5cp-25f))
#define NegPio2_3 (v_f32 (0x1.ee59dap-50f))
#define InvPio2 (v_f32 (0x1.45f306p-1f))
#define RangeVal (v_f32 (0x1p17f))
#define Shift (v_f32 (0x1.8p+23f))

#define poly(i) v_f32 (__tanf_poly_data.poly_tan[i])

/* Special cases (fall back to scalar calls).  */
VPCS_ATTR
NOINLINE static v_f32_t
specialcase (v_f32_t x, v_f32_t y, v_u32_t cmp)
{
  return v_call_f32 (tanf, x, y, cmp);
}

/* Use a full Estrin scheme to evaluate polynomial.  */
static inline v_f32_t
eval_poly (v_f32_t z)
{
  v_f32_t z2 = z * z;
  v_f32_t z4 = z2 * z2;
  return ESTRIN_6 (z, z2, z4, poly);
}

/* Fast implementation of Neon tanf.
   Maximum measured error: 3.121ulps.
   vtanq_f32(0x1.ff3df8p+16) got -0x1.fbb7b8p-1
			    want -0x1.fbb7b2p-1.  */
VPCS_ATTR
v_f32_t V_NAME (tanf) (v_f32_t x)
{
  /* Determine whether input is too large to perform fast regression.  */
  v_u32_t cmp = v_cage_f32 (x, RangeVal);

  /* n = rint(x/(pi/2)).  */
  v_f32_t q = v_fma_f32 (InvPio2, x, Shift);
  v_f32_t n = q - Shift;
  /* n is representable as a signed integer, simply convert it.  */
  v_s32_t in = v_round_s32 (n);
  /* Determine if x lives in an interval, where |tan(x)| grows to infinity.  */
  v_s32_t alt = in & 1;
  v_u32_t pred_alt = (alt != 0);

  /* r = x - n * (pi/2)  (range reduction into -pi./4 .. pi/4).  */
  v_f32_t r;
  r = v_fma_f32 (NegPio2_1, n, x);
  r = v_fma_f32 (NegPio2_2, n, r);
  r = v_fma_f32 (NegPio2_3, n, r);

  /* If x lives in an interval, where |tan(x)|
     - is finite, then use a polynomial approximation of the form
       tan(r) ~ r + r^3 * P(r^2) = r + r * r^2 * P(r^2).
     - grows to infinity then use symmetries of tangent and the identity
       tan(r) = cotan(pi/2 - r) to express tan(x) as 1/tan(-r). Finally, use
       the same polynomial approximation of tan as above.  */

  /* Perform additional reduction if required.  */
  v_f32_t z = v_sel_f32 (pred_alt, -r, r);

  /* Evaluate polynomial approximation of tangent on [-pi/4, pi/4].  */
  v_f32_t z2 = r * r;
  v_f32_t p = eval_poly (z2);
  v_f32_t y = v_fma_f32 (z * z2, p, z);

  /* Compute reciprocal and apply if required.  */
  v_f32_t inv_y = v_div_f32 (v_f32 (1.0f), y);
  y = v_sel_f32 (pred_alt, inv_y, y);

  /* Fast reduction does not handle the x = -0.0 case well,
     therefore it is fixed here.  */
  y = v_sel_f32 (x == v_f32 (-0.0), x, y);

  /* No need to pass pg to specialcase here since cmp is a strict subset,
     guaranteed by the cmpge above.  */
  if (unlikely (v_any_u32 (cmp)))
    return specialcase (x, y, cmp);
  return y;
}
VPCS_ALIAS
#endif
