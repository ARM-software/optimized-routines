/*
 * Single-precision vector erfc(x) function.
 *
 * Copyright (c) 2021-2023, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "v_math.h"
#include "erfcf.h"
#include "estrin.h"
#include "pl_sig.h"
#include "pl_test.h"

#define P(ia12) __erfcf_poly_data.poly[interval_index (ia12)]

VPCS_ATTR float64x2_t __v_exp_tail (float64x2_t, float64x2_t);

#ifndef SCALAR
static VPCS_ATTR NOINLINE float32x4_t
specialcase (float32x4_t x, float32x4_t y, uint32x4_t special)
{
  return v_call_f32 (erfcf, x, y, special);
}
#endif

static inline uint32_t
interval_index (uint32_t ia12)
{
  // clang-format off
  return (ia12 < 0x400 ? 0 :
         (ia12 < 0x408 ? 1 :
         (ia12 < 0x410 ? 2 :
                         3)));
  // clang-format on
}

/* The C macro wraps the coeffs argument in order to make the
   poynomial evaluation more readable.  */
#define C(i) ((float64x2_t){coeff1[i], coeff2[i]})

static inline float64x2_t
v_approx_erfcf_poly_gauss (float64x2_t x, const double *coeff1,
			   const double *coeff2)
{
  float64x2_t x2 = x * x;
  float64x2_t x4 = x2 * x2;
  float64x2_t poly = ESTRIN_15 (x, x2, x4, x4 * x4, C);
  float64x2_t gauss = __v_exp_tail (-(x * x), v_f64 (0.0));
  return poly * gauss;
}

static inline float
approx_poly_gauss (float abs_x, const double *coeff)
{
  return (float) (eval_poly (abs_x, coeff) * eval_exp_mx2 (abs_x));
}

static float32x4_t
v_approx_erfcf (float32x4_t abs_x, uint32x4_t sign, uint32x4_t ia12,
		uint32x4_t lanes)
{
  float32x2_t lo32 = {0, 0};
  float32x2_t hi32 = {0, 0};
  /* The polynomial and Gaussian components must be calculated in
     double precision in order to meet the required ULP error. This
     means we have to promote low and high halves of the
     single-precision input vector to two separate double-precision
     input vectors. This incurs some overhead, and there is also
     overhead to loading the polynomial coefficients as this cannot be
     done in a vector fashion. This would be wasted effort for
     elements which lie in the 'boring' zone, as they will be
     overwritten later. Hence we use the lanes parameter to only do
     the promotion on a pair of lanes if both of those lanes are
     interesting and not special cases. If one lane is inactive, we
     use a scalar routine which is shared with the scalar variant.  */
  if (lanes[0] & lanes[1])
    {
      lo32 = vcvt_f32_f64 (
	v_approx_erfcf_poly_gauss (vcvt_f64_f32 (vget_low_f32 (abs_x)),
				   P (ia12[0]), P (ia12[1])));
    }
  else if (lanes[0])
    {
      lo32[0] = approx_poly_gauss (abs_x[0], P (ia12[0]));
    }
  else if (lanes[1])
    {
      lo32[1] = approx_poly_gauss (abs_x[1], P (ia12[1]));
    }

  if (lanes[2] & lanes[3])
    {
      hi32
	= vcvt_f32_f64 (v_approx_erfcf_poly_gauss (vcvt_high_f64_f32 (abs_x),
						   P (ia12[2]), P (ia12[3])));
    }
  else if (lanes[2])
    {
      hi32[0] = approx_poly_gauss (abs_x[2], P (ia12[2]));
    }
  else if (lanes[3])
    {
      hi32[1] = approx_poly_gauss (abs_x[3], P (ia12[3]));
    }

  float32x4_t y = vcombine_f32 (lo32, hi32);

  if (v_any_u32 (sign))
    {
      y = vbslq_f32 (vceqzq_u32 (sign), y, 2 - y);
    }

  return y;
}

/* Optimized single-precision vector complementary error function
   erfcf. Max measured error: 0.750092 at various values between
   -0x1.06521p-20 and -0x1.add1dap-17. For example:
   __v_erfc(-0x1.08185p-18) got 0x1.00004cp+0 want 0x1.00004ap+0
   +0.249908 ulp err 0.250092.  */
VPCS_ATTR
float32x4_t V_NAME_F1 (erfc) (float32x4_t x)
{
  uint32x4_t ix = v_as_u32_f32 (x);
  uint32x4_t ia = ix & 0x7fffffff;
  uint32x4_t ia12 = ia >> 20;
  uint32x4_t sign = ix >> 31;
  uint32x4_t inf_ia12 = v_u32 (0x7f8);

  uint32x4_t special_cases = (ia12 - 0x328) >= ((inf_ia12 & 0x7f8) - 0x328);
  uint32x4_t in_bounds = (ia < 0x408ccccd) | (~sign & (ix < 0x4120f5c3));
  float32x4_t boring_zone = v_as_f32_u32 (sign << 30);

  float32x4_t y = v_approx_erfcf (v_as_f32_u32 (ia), sign, ia12,
				  in_bounds & ~special_cases);

  y = vbslq_f32 (~in_bounds, boring_zone, y);

  if (unlikely (v_any_u32 (special_cases)))
    {
      return specialcase (x, y, special_cases);
    }

  return y;
}

PL_SIG (V, F, 1, erfc, -6.0, 28.0)
PL_TEST_ULP (V_NAME_F1 (erfc), 0.26)
PL_TEST_INTERVAL (V_NAME_F1 (erfc), 0, 0xffff0000, 10000)
PL_TEST_INTERVAL (V_NAME_F1 (erfc), 0x1p-127, 0x1p-26, 40000)
PL_TEST_INTERVAL (V_NAME_F1 (erfc), -0x1p-127, -0x1p-26, 40000)
PL_TEST_INTERVAL (V_NAME_F1 (erfc), 0x1p-26, 0x1p5, 40000)
PL_TEST_INTERVAL (V_NAME_F1 (erfc), -0x1p-26, -0x1p3, 40000)
PL_TEST_INTERVAL (V_NAME_F1 (erfc), 0, inf, 40000)
