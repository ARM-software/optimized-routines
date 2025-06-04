/*
 * Double-precision SVE sinh(x) function.
 *
 * Copyright (c) 2023-2025, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "sv_math.h"
#include "test_sig.h"
#include "test_defs.h"

static const struct data
{
  uint64_t halff;
  double c2, c4;
  double inv_ln2;
  double ln2_hi, ln2_lo;
  double c0, c1, c3;
  double shift, special_bound, bound;
  uint64_t expm1_data[20];
} data = {
  /* Table lookup of 2^(i/64) - 1, for values of i from 0..19.  */
  .expm1_data = {
    0x0000000000000000, 0x3f864d1f3bc03077, 0x3f966c34c5615d0f, 0x3fa0e8a30eb37901,
    0x3fa6ab0d9f3121ec, 0x3fac7d865a7a3440, 0x3fb1301d0125b50a, 0x3fb429aaea92ddfb,
    0x3fb72b83c7d517ae, 0x3fba35beb6fcb754, 0x3fbd4873168b9aa8, 0x3fc031dc431466b2,
    0x3fc1c3d373ab11c3, 0x3fc35a2b2f13e6e9, 0x3fc4f4efa8fef709, 0x3fc6942d3720185a,
    0x3fc837f0518db8a9, 0x3fc9e0459320b7fa, 0x3fcb8d39b9d54e55, 0x3fcd3ed9a72cffb7,
  },

  /* Generated using Remez, in [-log(2)/128, log(2)/128].  */
  .c0 = 0x1p-1,
  .c1 = 0x1.55555555548f9p-3,
  .c2 = 0x1.5555555554c22p-5,
  .c3 = 0x1.111123aaa2fb2p-7,
  .c4 = 0x1.6c16d77d98e5bp-10,
  .ln2_hi = 0x1.62e42fefa3800p-1,
  .ln2_lo = 0x1.ef35793c76730p-45,
  .inv_ln2 = 0x1.71547652b82fep+0,
  .shift = 0x1.800000000ffc0p+46, /* 1.5*2^46+1023.  */
  .halff = 0x3fe0000000000000,
  .special_bound = 0x1.62e37e7d8ba72p+9,	/* ln(2^(1024 - 1/128)).  */
  .bound = 0x1.a56ef8ec924ccp-3 /* 19*ln2/64.  */
};

/* A specialised FEXPA expm1 that is only valid for positive inputs and
   has no special cases. Based off the full FEXPA expm1 implementated for
   _ZGVsMxv_expm1, with a slightly modified file to keep sinh under 3.5ULP.  */
static inline svfloat64_t
expm1_inline (svbool_t pg, svfloat64_t x)
{
  const struct data *d = ptr_barrier (&data);

  svfloat64_t z = svmla_x (pg, sv_f64 (d->shift), x, d->inv_ln2);
  svuint64_t u = svreinterpret_u64 (z);
  svfloat64_t n = svsub_x (pg, z, d->shift);

  svfloat64_t ln2 = svld1rq (svptrue_b64 (), &d->ln2_hi);
  svfloat64_t c24 = svld1rq (svptrue_b64 (), &d->c2);

  svfloat64_t r = x;
  r = svmls_lane (r, n, ln2, 0);
  r = svmls_lane (r, n, ln2, 1);

  svfloat64_t r2 = svmul_x (svptrue_b64 (), r, r);

  svfloat64_t p;
  svfloat64_t c12 = svmla_lane (sv_f64 (d->c1), r, c24, 0);
  svfloat64_t c34 = svmla_lane (sv_f64 (d->c3), r, c24, 1);
  p = svmad_x (pg, c34, r2, c12);
  p = svmad_x (pg, p, r, sv_f64 (d->c0));
  p = svmad_x (pg, p, r2, r);

  svfloat64_t scale = svexpa (u);

  /* We want to construct expm1(x) = (scale - 1) + scale * poly.
     However, for values of scale close to 1, scale-1 causes large ULP errors
     due to cancellation.

     This can be circumvented by using a small lookup for scale-1
     when our input is below a certain bound, otherwise we can use FEXPA.  */
  svbool_t is_small = svaclt (pg, x, d->bound);

  /* Index via the input of FEXPA, but we only care about the lower 5 bits.  */
  svuint64_t base_idx = svand_x (pg, u, 0x1f);

  /* Compute scale - 1 from FEXPA, and lookup values where this fails.  */
  svfloat64_t scalem1_estimate = svsub_x (pg, scale, sv_f64 (1.0));
  svuint64_t scalem1_lookup
      = svld1_gather_index (is_small, d->expm1_data, base_idx);

  /* Select the appropriate scale - 1 value based on x.  */
  svfloat64_t scalem1
      = svsel (is_small, svreinterpret_f64 (scalem1_lookup), scalem1_estimate);

  /* return expm1 = scale - 1 + (scale * poly).  */
  return svmla_x (pg, scalem1, scale, p);
}

/* Vectorised special case to handle values past where exp_inline overflows.
   Halves the input value and uses the identity exp(x) = exp(x/2)^2 to double
   the valid range of inputs, and returns inf for anything past that.  */
static svfloat64_t NOINLINE
special_case (svbool_t pg, svbool_t special, svfloat64_t ax,
	      svfloat64_t halfsign, const struct data *d)
{
  /* Halves input value, and then check if any cases
     are still going to overflow.  */
  ax = svmul_x (special, ax, 0.5);
  svbool_t is_safe = svaclt (special, ax, d->special_bound);

  svfloat64_t t = expm1_inline (pg, ax);

  /* Finish fastpass to compute values for non-special cases.  */
  svfloat64_t y = svadd_x (pg, t, svdiv_x (pg, t, svadd_x (pg, t, 1.0)));
  y = svmul_x (pg, y, halfsign);

  /* Computes special lane, and set remaining overflow lanes to inf.  */
  svfloat64_t half_special_y = svmul_x (svptrue_b64 (), t, halfsign);
  svfloat64_t special_y = svmul_x (svptrue_b64 (), half_special_y, t);

  svuint64_t signed_inf
      = svorr_x (svptrue_b64 (), svreinterpret_u64 (halfsign),
		 sv_u64 (0x7ff0000000000000));
  special_y = svsel (is_safe, special_y, svreinterpret_f64 (signed_inf));

  /* Join resulting vectors together and return.  */
  return svsel (special, special_y, y);
}

/* Approximation for SVE double-precision sinh(x) using FEXPA expm1.
   Uses sinh(x) = e^2x - 1 / 2e^x, rewritten for accuracy.
   The greatest observed error in the fast pass is 2.63 + 0.5 ULP:
   _ZGVsMxv_sinh (0x1.b5e0e13ba88aep-2) got 0x1.c3587faf97b0cp-2
				       want 0x1.c3587faf97b09p-2

   The greatest observed error in the slow pass is 2.65 + 0.5 ULP:
   _ZGVsMxv_sinh (0x1.633ce847dab1ap+9) got 0x1.fffd30eea0066p+1023
				       want 0x1.fffd30eea0063p+1023.  */
svfloat64_t SV_NAME_D1 (sinh) (svfloat64_t x, svbool_t pg)
{
  const struct data *d = ptr_barrier (&data);

  svbool_t special = svacge (pg, x, d->special_bound);
  svfloat64_t ax = svabs_x (pg, x);
  svuint64_t sign
      = sveor_x (pg, svreinterpret_u64 (x), svreinterpret_u64 (ax));
  svfloat64_t halfsign = svreinterpret_f64 (svorr_x (pg, sign, d->halff));

  /* Fall back to scalar variant for all lanes if any are special.  */
  if (unlikely (svptest_any (pg, special)))
    return special_case (pg, special, ax, halfsign, d);

  /* Up to the point that expm1 overflows, we can use it to calculate sinh
     using a slight rearrangement of the definition of sinh. This allows us to
     retain acceptable accuracy for very small inputs.  */
  svfloat64_t t = expm1_inline (pg, ax);
  t = svadd_x (pg, t, svdiv_x (pg, t, svadd_x (pg, t, 1.0)));
  return svmul_x (pg, t, halfsign);
}

TEST_SIG (SV, D, 1, sinh, -10.0, 10.0)
TEST_ULP (SV_NAME_D1 (sinh), 2.63)
TEST_DISABLE_FENV (SV_NAME_D1 (sinh))
TEST_SYM_INTERVAL (SV_NAME_D1 (sinh), 0, 0x1p-26, 1000)
TEST_SYM_INTERVAL (SV_NAME_D1 (sinh), 0x1p-26, 0x1p9, 500000)
TEST_SYM_INTERVAL (SV_NAME_D1 (sinh), 0x1p9, inf, 1000)
CLOSE_SVE_ATTR