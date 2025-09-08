/*
 * Double-precision vector sinpi function.
 *
 * Copyright (c) 2023-2025, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "mathlib.h"
#include "v_math.h"
#include "v_poly_f64.h"
#include "test_sig.h"
#include "test_defs.h"

static const struct data
{
  float64x2_t poly[10];
} data = {
  /* Polynomial coefficients generated using Remez algorithm,
     see sinpi.sollya for details.  */
  .poly = { V2 (0x1.921fb54442d184p1), V2 (-0x1.4abbce625be53p2),
	    V2 (0x1.466bc6775ab16p1), V2 (-0x1.32d2cce62dc33p-1),
	    V2 (0x1.507834891188ep-4), V2 (-0x1.e30750a28c88ep-8),
	    V2 (0x1.e8f48308acda4p-12), V2 (-0x1.6fc0032b3c29fp-16),
	    V2 (0x1.af86ae521260bp-21), V2 (-0x1.012a9870eeb7dp-25) },
};

/* Approximation for vector double-precision sinpi(x).
   Maximum Error 3.05 ULP:
  _ZGVnN2v_sinpi(0x1.d32750db30b4ap-2) got 0x1.fb295878301c7p-1
				      want 0x1.fb295878301cap-1.  */
float64x2_t VPCS_ATTR V_NAME_D1 (sinpi) (float64x2_t x)
{
  const struct data *d = ptr_barrier (&data);
  /* If r is odd, the sign of the result should be inverted.  */
  uint64x2_t odd
      = vshlq_n_u64 (vreinterpretq_u64_s64 (vcvtaq_s64_f64 (x)), 63);

  /* r = x - rint(x). Range reduction to -1/2 .. 1/2.  */
  float64x2_t r = vsubq_f64 (x, vrndaq_f64 (x));

  /* y = sin(r).  */
  float64x2_t r2 = vmulq_f64 (r, r);
  float64x2_t r4 = vmulq_f64 (r2, r2);
  float64x2_t y = vmulq_f64 (v_pw_horner_9_f64 (r2, r4, d->poly), r);

  return vreinterpretq_f64_u64 (veorq_u64 (vreinterpretq_u64_f64 (y), odd));
}

#if WANT_C23_TESTS
TEST_ULP (V_NAME_D1 (sinpi), 2.56)
TEST_SYM_INTERVAL (V_NAME_D1 (sinpi), 0, 0x1p-63, 5000)
TEST_SYM_INTERVAL (V_NAME_D1 (sinpi), 0x1p-63, 0.5, 10000)
TEST_SYM_INTERVAL (V_NAME_D1 (sinpi), 0.5, 0x1p51, 10000)
TEST_SYM_INTERVAL (V_NAME_D1 (sinpi), 0x1p51, inf, 10000)
#endif
