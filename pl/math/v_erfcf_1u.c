/*
 * Single-precision vector erfc(x) function.
 *
 * Copyright (c) 2021-2022, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "v_math.h"
#include "erfcf.h"

#if V_SUPPORTED

#define P(ia12) __erfcf_poly_data.poly[interval_index (ia12)]

VPCS_ATTR v_f64_t V_NAME (exp_tail) (v_f64_t, v_f64_t);

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
   poynomial evaluation more readable. In the scalarised variant the
   second pointer is ignored.  */
#ifdef SCALAR
#define C(i) coeff1[i]
#else
#define C(i) ((v_f64_t){coeff1[i], coeff2[i]})
#endif

static inline v_f64_t
v_eval_poly_estrin (v_f64_t z, const double *coeff1, const double *coeff2)
{
  v_f64_t z2 = z * z;
  v_f64_t z4 = z2 * z2;
  v_f64_t z8 = z4 * z4;

  v_f64_t c0_zc1 = v_fma_f64 (z, C (1), C (0));
  v_f64_t c2_zc3 = v_fma_f64 (z, C (3), C (2));
  v_f64_t c4_zc5 = v_fma_f64 (z, C (5), C (4));
  v_f64_t c6_zc7 = v_fma_f64 (z, C (7), C (6));
  v_f64_t c8_zc9 = v_fma_f64 (z, C (9), C (8));
  v_f64_t c10_zc11 = v_fma_f64 (z, C (11), C (10));
  v_f64_t c12_zc13 = v_fma_f64 (z, C (13), C (12));
  v_f64_t c14_zc15 = v_fma_f64 (z, C (15), C (14));

  v_f64_t c0_z2c3 = v_fma_f64 (z2, c2_zc3, c0_zc1);
  v_f64_t c4_z2c7 = v_fma_f64 (z2, c6_zc7, c4_zc5);
  v_f64_t c8_z2c11 = v_fma_f64 (z2, c10_zc11, c8_zc9);
  v_f64_t c12_z2c15 = v_fma_f64 (z2, c14_zc15, c12_zc13);

  v_f64_t c0_z4c7 = v_fma_f64 (z4, c4_z2c7, c0_z2c3);
  v_f64_t c8_z4c15 = v_fma_f64 (z4, c12_z2c15, c8_z2c11);

  return v_fma_f64 (z8, c8_z4c15, c0_z4c7);
}

#undef C

static inline v_f64_t
v_approx_erfcf_poly_gauss (v_f64_t x, const double *coeff1,
			   const double *coeff2)
{
  v_f64_t poly = v_eval_poly_estrin (x, coeff1, coeff2);
  v_f64_t gauss = V_NAME (exp_tail) (-(x * x), v_f64 (0.0));
  return poly * gauss;
}

static inline float
approx_poly_gauss (float abs_x, const double *coeff)
{
  return (float) (eval_poly_horner_lvl2 (abs_x, coeff) * eval_exp_mx2 (abs_x));
}

static v_f32_t
v_approx_erfcf (v_f32_t abs_x, v_u32_t sign, v_u32_t ia12, v_u32_t lanes)
{
#ifdef SCALAR
  float y = approx_poly_gauss (abs_x, P (ia12));
  return sign ? 2 - y : y;
#else
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

  v_f32_t y = vcombine_f32 (lo32, hi32);

  if (v_any_u32 (sign))
    {
      y = vbslq_f32 (vceqzq_u32 (sign), y, 2 - y);
    }

  return y;
#endif
}

/* Optimized single-precision vector complementary error function
   erfcf. Max measured error: 0.750092 at various values between
   -0x1.06521p-20 and -0x1.add1dap-17. For example:
   __v_erfc(-0x1.08185p-18) got 0x1.00004cp+0 want 0x1.00004ap+0
   +0.249908 ulp err 0.250092.  */
VPCS_ATTR
v_f32_t V_NAME (erfcf) (v_f32_t x)
{
  v_u32_t ix = v_as_u32_f32 (x);
  v_u32_t ia = ix & 0x7fffffff;
  v_u32_t ia12 = ia >> 20;
  v_u32_t sign = ix >> 31;
  v_u32_t inf_ia12 = v_u32 (0x7f8);

  v_u32_t special_cases
    = v_cond_u32 ((ia12 - 0x328) >= ((inf_ia12 & 0x7f8) - 0x328));
  v_u32_t in_bounds
    = v_cond_u32 ((ia < 0x408ccccd) | (~sign & (ix < 0x4120f5c3)));
  v_f32_t boring_zone = v_as_f32_u32 (sign << 30);

#ifdef SCALAR
  if (unlikely (special_cases))
    {
      if (ia12 >= 0x7f8)
	return (float) (sign << 1) + 1.0f / x; /* Special cases.  */
      else
	return 1.0f - x; /* Small case.  */
    }
  else if (likely (!in_bounds))
    {
      return sign ? boring_zone : __math_uflowf (boring_zone);
    }
#endif

  v_f32_t y = v_approx_erfcf (v_as_f32_u32 (ia), sign, ia12,
			      in_bounds & ~special_cases);

#ifndef SCALAR
  y = vbslq_f32 (~in_bounds, boring_zone, y);

  if (unlikely (v_any_u32 (special_cases)))
    {
      y = v_call_f32 (erfcf, x, y, special_cases);
    }
#endif

  return y;
}
VPCS_ALIAS
#endif
