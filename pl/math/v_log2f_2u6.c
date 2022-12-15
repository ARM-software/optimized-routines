/*
 * Single-precision vector log2 function.
 *
 * Copyright (c) 2022, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "v_math.h"
#include "math_config.h"
#include "pl_sig.h"
#include "pl_test.h"

#if V_SUPPORTED

#define N (1 << V_LOG2F_TABLE_BITS)
#define T __v_log2f_data.tab
#define A __v_log2f_data.poly
#define OFF 0x3f330000
#define SubnormLim 0x800000
#define One v_u32 (0x3f800000)

static float
handle_special (float x)
{
  if (x != x)
    /* NaN - return NaN but do not trigger invalid.  */
    return x;
  if (x < 0)
    /* log2f(-anything) = NaN.  */
    return __math_invalidf (x);
  if (x == 0)
    /* log2f(0) = Inf.  */
    return __math_divzerof (1);
  /* log2f(Inf)  =  Inf.  */
  return x;
}

static float
normalise (float x)
{
  return asfloat (asuint (x * 0x1p23f) - (23 << 23));
}

#ifdef SCALAR

#define DEFINE_LOOKUP_FUNC(p)                                                  \
  static inline float lookup_##p (uint32_t i) { return T[i].p; }

#else

#define DEFINE_LOOKUP_FUNC(p)                                                  \
  static inline v_f32_t lookup_##p (v_u32_t i)                                 \
  {                                                                            \
    return (v_f32_t){T[i[0]].p, T[i[1]].p, T[i[2]].p, T[i[3]].p};              \
  }

#endif

DEFINE_LOOKUP_FUNC (invc_lo)
DEFINE_LOOKUP_FUNC (invc_hi)
DEFINE_LOOKUP_FUNC (logc)

/* Single-precision vector log2 routine. Implements the same algorithms as
   scalar log2f, but using only single-precision arithmetic, with invc
   represented as a two-limb float. Accurate to 2.6 ulp. The maximum error is
   near sqrt(2):
  __v_log2f(0x1.6a0484p+0) got 0x1.ffea02p-2
			  want 0x1.ffea08p-2.  */
VPCS_ATTR v_f32_t V_NAME (log2f) (v_f32_t x)
{
  v_u32_t ix = v_as_u32_f32 (x);

  /* x is +-Inf, +-NaN, 0 or -ve.  */
  v_u32_t special = v_cond_u32 (ix >= 0x7f800000) | v_cond_u32 (ix == 0);
  /* |x| < 2^126 (i.e. x is subnormal).  */
  v_u32_t subnorm = v_cond_u32 (ix < SubnormLim);

  /* Sidestep special lanes so they do not inadvertently trigger fenv
     exceptions. They will be fixed up later.  */
  if (unlikely (v_any_u32 (special)))
    ix = v_sel_u32 (special, One, ix);

  if (unlikely (v_any_u32 (subnorm)))
    {
      /* Normalize any subnormals.  */
      v_f32_t tmp_x = v_as_f32_u32 (ix);
      ix = v_as_u32_f32 (v_call_f32 (normalise, tmp_x, tmp_x, subnorm));
    }

  /* x = 2^k z; where z is in range [OFF,2*OFF] and exact.
     The range is split into N subintervals.
     The ith subinterval contains z and c is near its center.  */
  v_u32_t tmp = ix - OFF;
  v_u32_t i = (tmp >> (23 - V_LOG2F_TABLE_BITS)) % N;
  v_u32_t top = tmp & 0xff800000;
  v_u32_t iz = ix - top;
  v_f32_t k = v_to_f32_s32 (v_as_s32_u32 (tmp) >> 23); /* Arithmetic shift.  */
  v_f32_t z = v_as_f32_u32 (iz);

  v_f32_t invc_lo = lookup_invc_lo (i);
  v_f32_t invc_hi = lookup_invc_hi (i);
  v_f32_t logc = lookup_logc (i);

  /* log2(x) = log1p(z/c-1)/ln2 + log2(c) + k.  */
  v_f32_t r = v_fma_f32 (z, invc_hi, v_f32 (-1));
  r = v_fma_f32 (z, invc_lo, r);
  v_f32_t y0 = logc + k;

  /* Pipelined polynomial evaluation to approximate log1p(r)/ln2.  */
  v_f32_t r2 = r * r;
  v_f32_t y = v_fma_f32 (v_f32 (A[1]), r, v_f32 (A[2]));
  y = v_fma_f32 (v_f32 (A[0]), r2, y);
  v_f32_t p = v_fma_f32 (v_f32 (A[3]), r, y0);
  y = v_fma_f32 (y, r2, p);

  if (unlikely (v_any_u32 (special)))
    return v_call_f32 (handle_special, x, y, special);

  return y;
}
VPCS_ALIAS

PL_SIG (V, F, 1, log2, 0.01, 11.1)
PL_TEST_ULP (V_NAME (log2f), 2.10)
PL_TEST_EXPECT_FENV (V_NAME (log2f), WANT_ERRNO)
PL_TEST_INTERVAL (V_NAME (log2f), -0.0, -0x1p126, 100)
PL_TEST_INTERVAL (V_NAME (log2f), 0x1p-149, 0x1p-126, 4000)
PL_TEST_INTERVAL (V_NAME (log2f), 0x1p-126, 0x1p-23, 50000)
PL_TEST_INTERVAL (V_NAME (log2f), 0x1p-23, 1.0, 50000)
PL_TEST_INTERVAL (V_NAME (log2f), 1.0, 100, 50000)
PL_TEST_INTERVAL (V_NAME (log2f), 100, inf, 50000)
#endif
