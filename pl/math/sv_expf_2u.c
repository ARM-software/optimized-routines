/*
 * Single-precision vector e^x function.
 *
 * Copyright (c) 2019-2022, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "sv_math.h"
#if SV_SUPPORTED

#define C(i) __sv_expf_poly[i]

#define Shift (0x1.8p23f) /* 1.5 * 2^23.  */
#define InvLn2 (0x1.715476p+0f)
#define Ln2hi (0x1.62e4p-1f)
#define Ln2lo (0x1.7f7d1cp-20f)
#define Thres (126.0f)

/* Update of both special and non-special cases, if any special case is
   detected.  */
static inline sv_f32_t
specialcase (svbool_t pg, sv_f32_t poly, sv_f32_t n, sv_u32_t e, sv_f32_t absn,
	     svbool_t p_cmp1, sv_f32_t scale)
{
  /* s=2^(n/N) may overflow, break it up into s=s1*s2,
     such that exp = s + s*y can be computed as s1*(s2+s2*y)
     and s1*s1 overflows only if n>0.  */

  /* If n<=0 then set b to 0x820...0, 0 otherwise.  */
  svbool_t p_sign = svcmple_n_f32 (pg, n, 0.0f); /* n <= 0.  */
  sv_u32_t b
    = svdup_n_u32_z (p_sign, 0x82000000); /* Inactive lanes set to 0.  */

  /* Set s1 to generate overflow depending on sign of exponent n.  */
  sv_f32_t s1
    = sv_as_f32_u32 (svadd_n_u32_x (pg, b, 0x7f000000)); /* b + 0x7f000000.  */
  /* Offset s to avoid overflow in final result if n is below threshold.  */
  sv_f32_t s2 = sv_as_f32_u32 (
    svsub_u32_x (pg, e, b)); /* as_u32 (s) - 0x3010...0 + b.  */

  /* |n| > 192 => 2^(n/N) overflows.  */
  svbool_t p_cmp2 = svcmpgt_n_f32 (pg, absn, 192.0f);

  sv_f32_t r2 = svmul_f32_x (pg, s1, s1);
  sv_f32_t r1 = sv_fma_f32_x (pg, poly, s2, s2);
  r1 = svmul_f32_x (pg, r1, s1);
  sv_f32_t r0 = sv_fma_f32_x (pg, poly, scale, scale);

  /* Apply condition 1 then 2.
     Returns r2 if cond2 is true, otherwise
     if cond1 is true then return r1, otherwise return r0.  */
  sv_f32_t r = svsel_f32 (p_cmp1, r1, r0);

  return svsel_f32 (p_cmp2, r2, r);
}

/* SVE port of single-precision vector exp routine from math/.
   Worst-case error is 1.95 ulp:
   __sv_expf(-0x1.4cb74ap+2) got 0x1.6a022cp-8
			     want 0x1.6a023p-8.  */
sv_f32_t
__sv_expf_x (sv_f32_t x, const svbool_t pg)
{
  /* exp(x) = 2^n (1 + poly(r)), with 1 + poly(r) in [1/sqrt(2),sqrt(2)]
     x = ln2*n + r, with r in [-ln2/2, ln2/2].  */

  /* n = round(x/(ln2/N)).  */
  sv_f32_t z = sv_fma_n_f32_x (pg, InvLn2, x, sv_f32 (Shift));
  sv_f32_t n = svsub_n_f32_x (pg, z, Shift);

  /* r = x - n*ln2/N.  */
  sv_f32_t r = sv_fma_n_f32_x (pg, -Ln2hi, n, x);
  r = sv_fma_n_f32_x (pg, -Ln2lo, n, r);

  /* u << 23.  */
  sv_u32_t e = svlsl_n_u32_x (pg, sv_as_u32_f32 (z), 23);

  /* s = 2^(n/N).  */
  sv_f32_t scale = sv_as_f32_u32 (svadd_n_u32_x (pg, e, 0x3f800000));
  sv_f32_t absn = svabs_f32_x (pg, n);

  svbool_t is_special_case = svcmpgt_n_f32 (pg, absn, Thres);

  /* y = exp(r) - 1 ~= r + C1 r^2 + C2 r^3 + C3 r^4.  */
  sv_f32_t r2 = svmul_f32_x (pg, r, r);
  sv_f32_t p = sv_fma_n_f32_x (pg, C (0), r, sv_f32 (C (1)));
  sv_f32_t q = sv_fma_n_f32_x (pg, C (2), r, sv_f32 (C (3)));
  q = sv_fma_f32_x (pg, p, r2, q);
  p = svmul_n_f32_x (pg, r, C (4));
  sv_f32_t poly = sv_fma_f32_x (pg, q, r2, p);

  /* The special case uses s, y and n to produce the final result (normal cases
     included). It performs an update of all lanes! Therefore:
     - all previous computation need to be done on all lanes indicated by input
     pg
     - we cannot simply apply the special case to the special-case-activated
     lanes. Besides it is likely that this would not increase performance (no
     scatter/gather).  */
  if (unlikely (svptest_any (pg, is_special_case)))
    return specialcase (pg, poly, n, e, absn, is_special_case, scale);

  return sv_fma_f32_x (pg, poly, scale, scale);
}

strong_alias (__sv_expf_x, _ZGVsMxv_expf)

#endif // SV_SUPPORTED
