/*
 * Single-precision AdvSIMD pow and powr function helpers.
 *
 * Copyright (c) 2025, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "v_math.h"

#define Log2IdxMask (V_POWF_LOG2_N - 1)
#define Exp2IdxMask (V_POWF_EXP2_N - 1)
#define Scale ((double) V_POWF_EXP2_N)
#define SignBias (1 << (V_POWF_EXP2_TABLE_BITS + 11))

#define MantissaMask 0x007fffff

static const struct data
{
  uint32x4_t one, special_bound, sign_bias;
  float32x4_t norm;
  uint32x4_t subnormal_bias;
  uint32x4_t off;
  float32x4_t uflow_bound, oflow_bound;
  uint32x4_t inf;
  float32x4_t nan;
  struct
  {
    double invc, logc;
  } log2_tab[V_POWF_LOG2_N];
  float64x2_t log2_poly[4];
  uint64_t exp2_tab[V_POWF_EXP2_N];
  float64x2_t exp2_poly[3];
} data = {
  /* Table and polynomial for log2 approximation.  */
  .log2_tab = {
    {0x1.6489890582816p+0, -0x1.e960f97b22702p-2 * Scale},
    {0x1.5cf19b35e3472p+0, -0x1.c993406cd4db6p-2 * Scale},
    {0x1.55aac0e956d65p+0, -0x1.aa711d9a7d0f3p-2 * Scale},
    {0x1.4eb0022977e01p+0, -0x1.8bf37bacdce9bp-2 * Scale},
    {0x1.47fcccda1dd1fp+0, -0x1.6e13b3519946ep-2 * Scale},
    {0x1.418ceabab68c1p+0, -0x1.50cb8281e4089p-2 * Scale},
    {0x1.3b5c788f1edb3p+0, -0x1.341504a237e2bp-2 * Scale},
    {0x1.3567de48e9c9ap+0, -0x1.17eaab624ffbbp-2 * Scale},
    {0x1.2fabc80fd19bap+0, -0x1.f88e708f8c853p-3 * Scale},
    {0x1.2a25200ce536bp+0, -0x1.c24b6da113914p-3 * Scale},
    {0x1.24d108e0152e3p+0, -0x1.8d02ee397cb1dp-3 * Scale},
    {0x1.1facd8ab2fbe1p+0, -0x1.58ac1223408b3p-3 * Scale},
    {0x1.1ab614a03efdfp+0, -0x1.253e6fd190e89p-3 * Scale},
    {0x1.15ea6d03af9ffp+0, -0x1.e5641882c12ffp-4 * Scale},
    {0x1.1147b994bb776p+0, -0x1.81fea712926f7p-4 * Scale},
    {0x1.0ccbf650593aap+0, -0x1.203e240de64a3p-4 * Scale},
    {0x1.0875408477302p+0, -0x1.8029b86a78281p-5 * Scale},
    {0x1.0441d42a93328p+0, -0x1.85d713190fb9p-6 * Scale},
    {0x1p+0, 0x0p+0 * Scale},
    {0x1.f1d006c855e86p-1, 0x1.4c1cc07312997p-5 * Scale},
    {0x1.e28c3341aa301p-1, 0x1.5e1848ccec948p-4 * Scale},
    {0x1.d4bdf9aa64747p-1, 0x1.04cfcb7f1196fp-3 * Scale},
    {0x1.c7b45a24e5803p-1, 0x1.582813d463c21p-3 * Scale},
    {0x1.bb5f5eb2ed60ap-1, 0x1.a936fa68760ccp-3 * Scale},
    {0x1.afb0bff8fe6b4p-1, 0x1.f81bc31d6cc4ep-3 * Scale},
    {0x1.a49badf7ab1f5p-1, 0x1.2279a09fae6b1p-2 * Scale},
    {0x1.9a14a111fc4c9p-1, 0x1.47ec0b6df5526p-2 * Scale},
    {0x1.901131f5b2fdcp-1, 0x1.6c71762280f1p-2 * Scale},
    {0x1.8687f73f6d865p-1, 0x1.90155070798dap-2 * Scale},
    {0x1.7d7067eb77986p-1, 0x1.b2e23b1d3068cp-2 * Scale},
    {0x1.74c2c1cf97b65p-1, 0x1.d4e21b0daa86ap-2 * Scale},
    {0x1.6c77f37cff2a1p-1, 0x1.f61e2a2f67f3fp-2 * Scale},
  },
  .log2_poly = { /* rel err: 1.5 * 2^-30.  */
    V2 (-0x1.6ff5daa3b3d7cp-2 * Scale),
    V2 (0x1.ec81d03c01aebp-2 * Scale),
    V2 (-0x1.71547bb43f101p-1 * Scale),
    V2 (0x1.7154764a815cbp0 * Scale)
  },
  /* Table and polynomial for exp2 approximation.  */
  .exp2_tab = {
    0x3ff0000000000000, 0x3fefd9b0d3158574, 0x3fefb5586cf9890f,
    0x3fef9301d0125b51, 0x3fef72b83c7d517b, 0x3fef54873168b9aa,
    0x3fef387a6e756238, 0x3fef1e9df51fdee1, 0x3fef06fe0a31b715,
    0x3feef1a7373aa9cb, 0x3feedea64c123422, 0x3feece086061892d,
    0x3feebfdad5362a27, 0x3feeb42b569d4f82, 0x3feeab07dd485429,
    0x3feea47eb03a5585, 0x3feea09e667f3bcd, 0x3fee9f75e8ec5f74,
    0x3feea11473eb0187, 0x3feea589994cce13, 0x3feeace5422aa0db,
    0x3feeb737b0cdc5e5, 0x3feec49182a3f090, 0x3feed503b23e255d,
    0x3feee89f995ad3ad, 0x3feeff76f2fb5e47, 0x3fef199bdd85529c,
    0x3fef3720dcef9069, 0x3fef5818dcfba487, 0x3fef7c97337b9b5f,
    0x3fefa4afa2a490da, 0x3fefd0765b6e4540,
  },
  .exp2_poly = { /* rel err: 1.69 * 2^-34.  */
    V2 (0x1.c6af84b912394p-5 / Scale / Scale / Scale),
    V2 (0x1.ebfce50fac4f3p-3 / Scale / Scale),
    V2 (0x1.62e42ff0c52d6p-1 / Scale),
  },
  .one = V4 (1),
  .special_bound = V4 (2u * 0x7f800000 - 1),
  .norm = V4 (0x1p23f),
  .subnormal_bias = V4 (0x0b800000), /* 23 << 23.  */
  .off = V4 (0x3f35d000),
  .sign_bias = V4 (SignBias),
  .inf = V4 (0x7f800000),
  .nan = V4 (__builtin_nanf ("")),
  /* 2.6 ulp ~ 0.5 + 2^24 (128*Ln2*relerr_log2 + relerr_exp2).  */
  .uflow_bound = V4 (-0x1.2cp+12f), /* -150.0 * V_POWF_EXP2_N.  */
  .oflow_bound = V4 (0x1p+12f), /* 128.0 * V_POWF_EXP2_N.  */
};

static inline float64x2_t
ylogx_core (const struct data *d, float64x2_t iz, float64x2_t k,
	    float64x2_t invc, float64x2_t logc, float64x2_t y)
{

  /* log2(x) = log1p(z/c-1)/ln2 + log2(c) + k.  */
  float64x2_t r = vfmaq_f64 (v_f64 (-1.0), iz, invc);
  float64x2_t y0 = vaddq_f64 (logc, k);

  /* Polynomial to approximate log1p(r)/ln2.  */
  float64x2_t logx = vfmaq_f64 (d->log2_poly[1], r, d->log2_poly[0]);
  logx = vfmaq_f64 (d->log2_poly[2], logx, r);
  logx = vfmaq_f64 (d->log2_poly[3], logx, r);
  logx = vfmaq_f64 (y0, logx, r);

  return vmulq_f64 (logx, y);
}

static inline float64x2_t
log2_lookup (const struct data *d, uint32_t i)
{
  return vld1q_f64 (
      &d->log2_tab[(i >> (23 - V_POWF_LOG2_TABLE_BITS)) & Log2IdxMask].invc);
}

static inline uint64x1_t
exp2_lookup (const struct data *d, uint64_t i)
{
  return vld1_u64 (&d->exp2_tab[i & Exp2IdxMask]);
}

static inline float64x2_t
exp2_core (const struct data *d, float64x2_t ylogx, uint64x2_t sign_bias)
{
  /* N*x = k + r with r in [-1/2, 1/2].  */
  float64x2_t kd = vrndnq_f64 (ylogx);
  int64x2_t ki = vcvtaq_s64_f64 (ylogx);
  float64x2_t r = vsubq_f64 (ylogx, kd);

  /* exp2(x) = 2^(k/N) * 2^r ~= s * (C0*r^3 + C1*r^2 + C2*r + 1).  */
  uint64x2_t t = vcombine_u64 (exp2_lookup (d, vgetq_lane_s64 (ki, 0)),
			       exp2_lookup (d, vgetq_lane_s64 (ki, 1)));
  int64x2_t ski = vaddq_s64 (ki, vreinterpretq_s64_u64 (sign_bias));
  t = vaddq_u64 (t, vreinterpretq_u64_s64 (
			vshlq_n_s64 (ski, 52 - V_POWF_EXP2_TABLE_BITS)));
  float64x2_t s = vreinterpretq_f64_u64 (t);
  float64x2_t p = vfmaq_f64 (d->exp2_poly[1], r, d->exp2_poly[0]);
  p = vfmaq_f64 (d->exp2_poly[2], r, p);
  p = vfmaq_f64 (s, p, vmulq_f64 (s, r));
  return p;
}

static inline float32x4_t
powrf_core (const struct data *d, float32x4_t *ylogx, uint32x4_t tmp,
	    float32x4_t iz, float32x4_t y, int32x4_t k, uint32x4_t sign_bias)
{
  /* Use double precision for each lane: split input vectors into lo and hi
     halves and promote.  */
  float64x2_t tab0 = log2_lookup (d, vgetq_lane_u32 (tmp, 0)),
	      tab1 = log2_lookup (d, vgetq_lane_u32 (tmp, 1)),
	      tab2 = log2_lookup (d, vgetq_lane_u32 (tmp, 2)),
	      tab3 = log2_lookup (d, vgetq_lane_u32 (tmp, 3));

  float64x2_t iz_lo = vcvt_f64_f32 (vget_low_f32 (iz)),
	      iz_hi = vcvt_high_f64_f32 (iz);

  float64x2_t k_lo = vcvtq_f64_s64 (vmovl_s32 (vget_low_s32 (k))),
	      k_hi = vcvtq_f64_s64 (vmovl_high_s32 (k));

  float64x2_t invc_lo = vzip1q_f64 (tab0, tab1),
	      invc_hi = vzip1q_f64 (tab2, tab3),
	      logc_lo = vzip2q_f64 (tab0, tab1),
	      logc_hi = vzip2q_f64 (tab2, tab3);

  float64x2_t y_lo = vcvt_f64_f32 (vget_low_f32 (y)),
	      y_hi = vcvt_high_f64_f32 (y);

  float64x2_t ylogx_lo = ylogx_core (d, iz_lo, k_lo, invc_lo, logc_lo, y_lo);
  float64x2_t ylogx_hi = ylogx_core (d, iz_hi, k_hi, invc_hi, logc_hi, y_hi);

  uint64x2_t sign_bias_lo = vmovl_u32 (vget_low_u32 (sign_bias));
  uint64x2_t sign_bias_hi = vmovl_high_u32 (sign_bias);

  float32x2_t p_lo = vcvt_f32_f64 (exp2_core (d, ylogx_lo, sign_bias_lo));
  float32x2_t p_hi = vcvt_f32_f64 (exp2_core (d, ylogx_hi, sign_bias_hi));

  *ylogx = vcombine_f32 (vcvt_f32_f64 (ylogx_lo), vcvt_f32_f64 (ylogx_hi));

  return vcombine_f32 (p_lo, p_hi);
}

/* Check if x is an integer.  */
static inline uint32x4_t
visint (float32x4_t x)
{
  return vceqq_f32 (vrndq_f32 (x), x);
}

/* Check if x is real not integer valued.  */
static inline uint32x4_t
visnotint (float32x4_t x)
{
  return vmvnq_u32 (vceqq_f32 (vrndq_f32 (x), x));
}

/* Check if x is an odd integer.  */
static inline uint32x4_t
visodd (float32x4_t x)
{
  float32x4_t y = vmulq_n_f32 (x, 0.5f);
  return visnotint (y);
}

/* Check if zero, inf or nan.  */
static inline uint32x4_t
v_zeroinfnan (const struct data *d, uint32x4_t i)
{
  return vcgeq_u32 (vsubq_u32 (vaddq_u32 (i, i), d->one), d->special_bound);
}

/* Returns 0 if not int, 1 if odd int, 2 if even int.  The argument is
   the bit representation of a non-zero finite floating-point value.  */
static inline int
checkint (uint32_t iy)
{
  int e = iy >> 23 & 0xff;
  if (e < 0x7f)
    return 0;
  if (e > 0x7f + 23)
    return 2;
  if (iy & ((1 << (0x7f + 23 - e)) - 1))
    return 0;
  if (iy & (1 << (0x7f + 23 - e)))
    return 1;
  return 2;
}

/* Check if zero, inf or nan.  */
static inline int
zeroinfnan (uint32_t ix)
{
  return 2 * ix - 1 >= 2u * 0x7f800000 - 1;
}

/* A scalar subroutine used to fix main power special cases. Similar to the
   preamble of scalar powf except that we do not update ix and sign_bias. This
   is done in the preamble of the SVE powf.  */
static inline float
powf_specialcase (float x, float y)
{
  uint32_t ix = asuint (x);
  uint32_t iy = asuint (y);
  /* Either x or y is 0 or inf or nan.  */
  if (unlikely (zeroinfnan (iy)))
    {
      if (2 * iy == 0)
	return issignalingf_inline (x) ? x + y : 1.0f;
      if (ix == 0x3f800000)
	return issignalingf_inline (y) ? x + y : 1.0f;
      if (2 * ix > 2u * 0x7f800000 || 2 * iy > 2u * 0x7f800000)
	return x + y;
      if (2 * ix == 2 * 0x3f800000)
	return 1.0f;
      if ((2 * ix < 2 * 0x3f800000) == !(iy & 0x80000000))
	return 0.0f; /* |x|<1 && y==inf or |x|>1 && y==-inf.  */
      return y * y;
    }
  if (unlikely (zeroinfnan (ix)))
    {
      float_t x2 = x * x;
      if (ix & 0x80000000 && checkint (iy) == 1)
	x2 = -x2;
      return iy & 0x80000000 ? 1 / x2 : x2;
    }
  return x;
}

/* Special case function wrapper.  */
static float32x4_t VPCS_ATTR NOINLINE
special_case (float32x4_t x, float32x4_t y, float32x4_t ret, uint32x4_t cmp)
{
  return v_call2_f32 (powf_specialcase, x, y, ret, cmp);
}

/* Power implementation without assumptions on x or y.
   Evaluate powr(|x|) = exp (y * log(|x|)), and handle
   sign of x using the sign bias.
   Handle underflow and overflow in exponential.  */
static inline float32x4_t
v_powf_core (float32x4_t x, float32x4_t y, uint32x4_t sign_bias,
	     const struct data *d)
{
  uint32x4_t ix = vreinterpretq_u32_f32 (x);

  /* Part of core computation carried in working precision.  */
  uint32x4_t tmp = vsubq_u32 (ix, d->off);
  uint32x4_t top = vbicq_u32 (tmp, v_u32 (MantissaMask));
  float32x4_t iz = vreinterpretq_f32_u32 (vsubq_u32 (ix, top));
  int32x4_t k
      = vshrq_n_s32 (vreinterpretq_s32_u32 (top),
		     23 - V_POWF_EXP2_TABLE_BITS); /* arithmetic shift.  */

  /* Compute core in extended precision and return intermediate ylogx results
     to handle cases of underflow and overflow in exp.  */
  float32x4_t ylogx;
  float32x4_t ret = powrf_core (d, &ylogx, tmp, iz, y, k, sign_bias);

  /* Handle exp special cases of underflow and overflow.  */
  uint32x4_t sign = vshlq_n_u32 (sign_bias, 20 - V_POWF_EXP2_TABLE_BITS);
  float32x4_t ret_oflow = vreinterpretq_f32_u32 (vorrq_u32 (sign, d->inf));
  float32x4_t ret_uflow = vreinterpretq_f32_u32 (sign);
  ret = vbslq_f32 (vcleq_f32 (ylogx, d->uflow_bound), ret_uflow, ret);
  ret = vbslq_f32 (vcgtq_f32 (ylogx, d->oflow_bound), ret_oflow, ret);
  return ret;
}

/* Power implementation for x containing negative or subnormal lanes.  */
static inline float32x4_t
v_powf_x_is_neg_or_small (float32x4_t x, float32x4_t y, const struct data *d)
{
  uint32x4_t xisneg = vcltzq_f32 (x);
  uint32x4_t xsmall = vcaltq_f32 (x, v_f32 (0x1p-126f));

  /* Set sign_bias depending on sign of x and nature of y.  */
  uint32x4_t yisodd_xisneg = vandq_u32 (visodd (y), xisneg);

  /* Set variable to SignBias if x is negative and y is odd.  */
  uint32x4_t sign_bias = vandq_u32 (d->sign_bias, yisodd_xisneg);

  /* Normalize subnormals.  */
  float32x4_t a = vabsq_f32 (x);
  uint32x4_t ia_norm = vreinterpretq_u32_f32 (vmulq_f32 (a, d->norm));
  ia_norm = vsubq_u32 (ia_norm, d->subnormal_bias);
  a = vbslq_f32 (xsmall, vreinterpretq_f32_u32 (ia_norm), a);

  /* Evaluate exp (y * log(x)) using |x| and sign bias correction.  */
  float32x4_t ret = v_powf_core (a, y, sign_bias, d);

  /* Cases of finite y and finite negative x.  */
  uint32x4_t yint_or_xpos = vornq_u32 (visint (y), xisneg);
  return vbslq_f32 (yint_or_xpos, ret, d->nan);
}

/* Top-level interface called from powf and powrf.  */
static inline float32x4_t
v_powf_inline (float32x4_t x, float32x4_t y)
{
  const struct data *d = ptr_barrier (&data);

  /* Special cases of x or y: zero, inf and nan.  */
  uint32x4_t ix = vreinterpretq_u32_f32 (x);
  uint32x4_t iy = vreinterpretq_u32_f32 (y);
  uint32x4_t xspecial = v_zeroinfnan (d, ix);
  uint32x4_t yspecial = v_zeroinfnan (d, iy);
  uint32x4_t cmp = vorrq_u32 (xspecial, yspecial);

  /* Evaluate pow(x, y) for x containing negative or subnormal lanes.  */
  uint32x4_t x_is_neg_or_sub = vcltq_f32 (x, v_f32 (0x1p-126f));
  if (unlikely (v_any_u32 (x_is_neg_or_sub)))
    {
      float32x4_t ret = v_powf_x_is_neg_or_small (x, y, d);
      if (unlikely (v_any_u32 (cmp)))
	return special_case (x, y, ret, cmp);
      return ret;
    }

  /* Else evaluate pow(x, y) for normal and positive x only.
     Sign bias is set to 0, which could be optimized further.  */
  uint32x4_t sign_bias = v_u32 (0);
  if (unlikely (v_any_u32 (cmp)))
    return special_case (x, y, v_powf_core (x, y, sign_bias, d), cmp);
  return v_powf_core (x, y, sign_bias, d);
}
