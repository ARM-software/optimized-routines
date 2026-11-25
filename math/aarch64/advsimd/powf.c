/*
 * Single-precision vector x^y function.
 *
 * Copyright (c) 2019-2026, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "test_defs.h"
#include "test_sig.h"
#include "v_math.h"
#include "v_powf_inline.h"

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

/* Check if x is odd, assuming x is an integer: x / 2 is not integral.
   Also true for non-integer x except +-0x1p-149 (x / 2 rounds to 0).  */
static inline uint32x4_t
visodd (float32x4_t x)
{
  float32x4_t y = vmulq_n_f32 (x, 0.5f);
  return visnotint (y);
}

/* pow (x, y) for special x (0, inf or nan) and finite nonzero y:
   x * x, or 1 / (x * x) for negative y.  For negative x and odd integer
   y, special_case also flips the sign.  x * x is exactly +0, +inf or
   NaN, so the reciprocal is an exponent-field flip (0 <-> inf, NaN
   unchanged) instead of a slow vector FDIV.  As a result
   pow (+-0, y < 0) does not raise FE_DIVBYZERO.  */
static inline float32x4_t
v_powf_xspecial (float32x4_t x, uint32x4_t iy)
{
  uint32x4_t ix = vreinterpretq_u32_f32 (x);
  uint32x4_t ix2 = vaddq_u32 (ix, ix);
  uint32x4_t x_nan = vcgtq_u32 (ix2, vdupq_n_u32 (2u * 0x7f800000));
  uint32x4_t y_neg = vcltzq_s32 (vreinterpretq_s32_u32 (iy));
  float32x4_t x2 = vmulq_f32 (x, x);
  /* 0x7f800000 where y < 0 and x is not NaN, built from the mask instead
     of a constant.  */
  uint32x4_t expflip
      = vshrq_n_u32 (vshlq_n_u32 (vbicq_u32 (y_neg, x_nan), 24), 1);
  return vreinterpretq_f32_u32 (
      veorq_u32 (vreinterpretq_u32_f32 (x2), expflip));
}

/* Fix the lanes where x or y is 0, inf or nan, with a lane-parallel
   transcription of the zeroinfnan branches of scalar powf.  In the y
   special branch, each conditional return becomes a select, applied in
   reverse order so that earlier conditions win.  The results are built
   from the same values as in the scalar code (1.0f, 0.0f, x + y, x * x,
   y * y).  Lanes not in CMP keep RET.  YSPECIAL marks the lanes of CMP
   where y is 0, inf or nan.  Computing every case may raise spurious FP
   exceptions.  Vector math does not guarantee exception flags.  */
static float32x4_t VPCS_ATTR NOINLINE
special_case (float32x4_t x, float32x4_t y, float32x4_t ret, uint32x4_t cmp,
	      uint32x4_t yspecial)
{
  uint32x4_t ix = vreinterpretq_u32_f32 (x);
  uint32x4_t iy = vreinterpretq_u32_f32 (y);
  /* 2 * i drops the sign bit, as in the scalar code.  */
  uint32x4_t ix2 = vaddq_u32 (ix, ix);
  uint32x4_t iy2 = vaddq_u32 (iy, iy);

  uint32x4_t inf2 = vdupq_n_u32 (2u * 0x7f800000);
  uint32x4_t one2 = vdupq_n_u32 (2u * 0x3f800000);
  uint32x4_t y_zero = vceqzq_u32 (iy2);
  uint32x4_t x_one = vceqq_u32 (ix, vdupq_n_u32 (0x3f800000));
  uint32x4_t x_nan = vcgtq_u32 (ix2, inf2);
  uint32x4_t y_nan = vcgtq_u32 (iy2, inf2);
  uint32x4_t any_nan = vorrq_u32 (x_nan, y_nan);
  uint32x4_t x_abs1 = vceqq_u32 (ix2, one2);
  uint32x4_t x_lt1 = vcltq_u32 (ix2, one2);
  uint32x4_t y_neg = vcltzq_s32 (vreinterpretq_s32_u32 (iy));
  uint32x4_t x_neg = vcltzq_s32 (vreinterpretq_s32_u32 (ix));

  /* A NaN is signaling if the top mantissa bit is clear.  */
  uint32x4_t quiet_bit = vdupq_n_u32 (0x00400000);
  uint32x4_t x_snan = vbicq_u32 (x_nan, vtstq_u32 (ix, quiet_bit));
  uint32x4_t y_snan = vbicq_u32 (y_nan, vtstq_u32 (iy, quiet_bit));

  /* The two branches of the scalar code cover disjoint lanes (y
     special, or only x special), so the order in which they are applied
     here does not matter.  Each runs only if some lane needs it.  */
  float32x4_t res = ret;

  /* Second scalar branch: x is 0, inf or nan, y is finite and nonzero.  */
  uint32x4_t xonly = vbicq_u32 (cmp, yspecial);
  if (v_any_u32 (xonly))
    {
      float32x4_t r_x = v_powf_xspecial (x, iy);
      /* Only negative special x needs the odd-integer test, the longest
	 dependency chain in this branch.  */
      if (v_any_u32 (vandq_u32 (x_neg, xonly)))
	{
	  /* checkint (iy) == 1.  visodd alone is also true for
	     non-integer y such as 2.5.  */
	  uint32x4_t y_odd = vandq_u32 (visint (y), visodd (y));
	  uint32x4_t flip = vandq_u32 (vandq_u32 (x_neg, y_odd),
				       vdupq_n_u32 (0x80000000));
	  r_x = vreinterpretq_f32_u32 (
	      veorq_u32 (vreinterpretq_u32_f32 (r_x), flip));
	}
      res = vbslq_f32 (xonly, r_x, res);
    }

  /* First scalar branch: y is 0, inf or nan.  */
  if (v_any_u32 (yspecial))
    {
      /* As in the scalar code, x + y quiets sNaNs and propagates NaNs.  */
      float32x4_t xpy = vaddq_f32 (x, y);
      float32x4_t r_y = vmulq_f32 (y, y);
      r_y = vbslq_f32 (veorq_u32 (x_lt1, y_neg), v_f32 (0.0f), r_y);
      r_y = vbslq_f32 (x_abs1, v_f32 (1.0f), r_y);
      r_y = vbslq_f32 (any_nan, xpy, r_y);
      r_y = vbslq_f32 (x_one, vbslq_f32 (y_snan, xpy, v_f32 (1.0f)), r_y);
      r_y = vbslq_f32 (y_zero, vbslq_f32 (x_snan, xpy, v_f32 (1.0f)), r_y);
      res = vbslq_f32 (yspecial, r_y, res);
    }

  return res;
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

/* Pack two lane masks (each lane 0 or -1u) into one scalar, one byte per
   lane, A in the low and B in the high 32 bits, so that both reach the
   general registers with a single transfer.  Lane i of A and of B land
   in the same byte of their halves, so the halves can be combined
   bitwise.  The instructions act on architectural lanes, so the layout
   is the same on big-endian.  */
static inline uint64_t
v_pack_masks (uint32x4_t a, uint32x4_t b)
{
  uint16x8_t ab = vuzp1q_u16 (vreinterpretq_u16_u32 (a),
			      vreinterpretq_u16_u32 (b));
  return vget_lane_u64 (vreinterpret_u64_u8 (vmovn_u16 (ab)), 0);
}

/* Handle a vector with at least one lane where x is negative, subnormal
   or special, or y is special.  MASKS holds the x < 0x1p-126 lanes in
   its low word and the special lanes in its high word.  Kept out of
   line so that the fast path has no stack frame and no callee-saved
   registers to preserve.  */
static float32x4_t VPCS_ATTR NOINLINE
v_powf_cold (float32x4_t x, float32x4_t y, uint32x4_t cmp,
	     uint32x4_t yspecial, const struct data *d, uint64_t masks)
{
  uint32x4_t ix = vreinterpretq_u32_f32 (x);
  uint32x4_t iy = vreinterpretq_u32_f32 (y);
  uint32_t lt_min_bits = masks;
  uint32_t cmp_bits = masks >> 32;

  /* v_powf_x_is_neg_or_small is needed only for negative or
     subnormal x that is not special.  Special lanes take their result
     from special_case or v_powf_xspecial anyway, and +0 is below
     0x1p-126, so without this an exact zero lane would send the whole
     vector through it.  */
  if (unlikely ((lt_min_bits & ~cmp_bits) != 0))
    {
      float32x4_t ret = v_powf_x_is_neg_or_small (x, y, d);
      if (unlikely (cmp_bits != 0))
	return special_case (x, y, ret, cmp, yspecial);
      return ret;
    }

  /* Every lane below 0x1p-126 is special here.  Special y, and
     negative special x (whose result sign depends on the parity of y),
     need the full decision tree.  */
  uint32x4_t x_neg = vcltzq_s32 (vreinterpretq_s32_u32 (ix));
  uint32x4_t need_tree = vorrq_u32 (yspecial, vandq_u32 (cmp, x_neg));
  if (unlikely (v_any_u32 (need_tree)))
    {
      /* Skip the core when special_case overwrites every lane.  */
      if (cmp_bits == 0xffffffff)
	return special_case (x, y, x, cmp, yspecial);
      return special_case (x, y, v_powrf_core (x, y, d), cmp, yspecial);
    }
  /* Only non-negative special x: fixed inline with v_powf_xspecial
     and one select, avoiding the call.  */
  float32x4_t r_x = v_powf_xspecial (x, iy);
  if (cmp_bits == 0xffffffff)
    return r_x;
  return vbslq_f32 (cmp, r_x, v_powrf_core (x, y, d));
}

/* Implementation of AdvSIMD powf.
   Maximum measured error is 2.57 ULPs:
   V_NAME_F2 (pow) (0x1.031706p+0, 0x1.ce2ec2p+12)
     got 0x1.fff868p+127
    want 0x1.fff862p+127.  */
float32x4_t VPCS_ATTR NOINLINE V_NAME_F2 (pow) (float32x4_t x, float32x4_t y)
{
  const struct data *d = ptr_barrier (&data);

  /* Special cases of x or y: zero, inf and nan.  */
  uint32x4_t ix = vreinterpretq_u32_f32 (x);
  uint32x4_t iy = vreinterpretq_u32_f32 (y);
  uint32x4_t xspecial = v_zeroinfnan (d, ix);
  uint32x4_t yspecial = v_zeroinfnan (d, iy);
  uint32x4_t cmp = vorrq_u32 (xspecial, yspecial);

  /* One unlikely branch covers both negative or subnormal x
     (x < 0x1p-126) and the special lanes (CMP).  The cold side tells
     them apart with scalar tests on the packed masks, because
     vector-to-general transfers are comparatively expensive on some
     cores.  */
  uint32x4_t x_lt_min = vcltq_f32 (x, v_f32 (0x1p-126f));
  uint64_t masks = v_pack_masks (x_lt_min, cmp);
  if (unlikely (masks != 0))
    return v_powf_cold (x, y, cmp, yspecial, d, masks);
  return v_powrf_core (x, y, d);
}

HALF_WIDTH_ALIAS_F2 (pow)

TEST_SIG (V, F, 2, pow)
TEST_ULP (V_NAME_F2 (pow), 2.08)
#define V_POWF_INTERVAL2(xlo, xhi, ylo, yhi, n)                               \
  TEST_INTERVAL2 (V_NAME_F2 (pow), xlo, xhi, ylo, yhi, n)                     \
  TEST_INTERVAL2 (V_NAME_F2 (pow), xlo, xhi, -ylo, -yhi, n)
/* Wide intervals spanning the whole domain.  */
V_POWF_INTERVAL2 (0, 0x1p-126, 0, inf, 40000)
V_POWF_INTERVAL2 (0x1p-126, 1, 0, inf, 50000)
V_POWF_INTERVAL2 (1, inf, 0, inf, 50000)
/* x~1 or y~1.  */
V_POWF_INTERVAL2 (0x1p-1, 0x1p1, 0x1p-7, 0x1p7, 50000)
V_POWF_INTERVAL2 (0x1p-70, 0x1p70, 0x1p-1, 0x1p1, 50000)
V_POWF_INTERVAL2 (0x1.ep-1, 0x1.1p0, 0x1p8, 0x1p14, 50000)
#define V_POWF_SPECIALX(ylo, yhi, n)                                          \
  V_POWF_INTERVAL2 (0, 0, ylo, yhi, n)                                        \
  V_POWF_INTERVAL2 (1, 1, ylo, yhi, n)                                        \
  V_POWF_INTERVAL2 (inf, inf, ylo, yhi, n)                                    \
  V_POWF_INTERVAL2 (nan, nan, ylo, yhi, n)                                    \
  V_POWF_INTERVAL2 (0xffff0000, 0xffff0000, ylo, yhi, n)
#define V_POWF_SPECIALY(xlo, xhi, n)                                          \
  V_POWF_INTERVAL2 (xlo, xhi, 0, 0, n)                                        \
  V_POWF_INTERVAL2 (xlo, xhi, inf, inf, n)                                    \
  V_POWF_INTERVAL2 (xlo, xhi, nan, nan, n)                                    \
  V_POWF_INTERVAL2 (xlo, xhi, 0xffff0000, 0xffff0000, n)
/* x is 0, inf or nan. |y| is finite.  */
V_POWF_SPECIALX (0.0, inf, 1000)
/* x is 0, inf or nan. |y| is special.  */
V_POWF_SPECIALX (0.0, 0.0, 1)
V_POWF_SPECIALX (inf, inf, 1)
V_POWF_SPECIALX (nan, nan, 1)
V_POWF_SPECIALX (0xffff0000, 0xffff0000, 1)
/* |y| is 0, inf or nan. x is finite.  */
V_POWF_SPECIALY (0.0, inf, 1000)
/* |y| is 0, inf or nan. x is special.  */
V_POWF_SPECIALY (0.0, 0.0, 1)
V_POWF_SPECIALY (1.0, 1.0, 1)
V_POWF_SPECIALY (inf, inf, 1)
V_POWF_SPECIALY (0xffff0000, 0xffff0000, 1)
/* x is negative.  */
TEST_INTERVAL2 (V_NAME_F2 (pow), -0.0, -inf, 0, 0xffff0000, 1000)

/* pow-specific cases, not shared with powr.  */

/* x is negative, y is odd or even integer, or y is real not integer.  */
V_POWF_INTERVAL2 (-0.0, -10.0, 1.0, 1.0, 1000)
V_POWF_INTERVAL2 (-0.0, -10.0, 3.0, 3.0, 1000)
V_POWF_INTERVAL2 (-0.0, -10.0, 4.0, 4.0, 1000)
V_POWF_INTERVAL2 (-0.0, -10.0, 0.0, 10.0, 1000)
V_POWF_INTERVAL2 (0.0, 10.0, 0.0, 10.0, 1000)
/* |x| is inf, y is odd or even integer, or y is real not integer.  */
V_POWF_INTERVAL2 (inf, inf, 0.5, 0.5, 1)
V_POWF_INTERVAL2 (inf, inf, 1.0, 1.0, 1)
V_POWF_INTERVAL2 (inf, inf, 2.0, 2.0, 1)
V_POWF_INTERVAL2 (inf, inf, 3.0, 3.0, 1)
