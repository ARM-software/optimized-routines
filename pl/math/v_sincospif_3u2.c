/*
 * Single-precision vector sincospi function.
 *
 * Copyright (c) 2024, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "v_sincospif_common.h"
#include "v_math.h"
#include "pl_test.h"
#include "mathlib.h"

/* Single-precision vector function allowing calculation of both sinpi and
   cospi in one function call, using shared argument reduction and polynomials.
   Worst-case error for sin is 3.04 ULP:
   _ZGVnN4v_sincospif_sin(0x1.1d341ap-1) got 0x1.f7cd56p-1 want 0x1.f7cd5p-1.
   Worst-case error for cos is 3.18 ULP:
   _ZGVnN4v_sincospif_cos(0x1.d341a8p-5) got 0x1.f7cd56p-1 want 0x1.f7cd5p-1.
 */
VPCS_ATTR void
_ZGVnN4vl4l4_sincospif (float32x4_t x, float32x4_t *out_sin,
			float32x4_t *out_cos)
{
  const struct v_sincospif_data *d = ptr_barrier (&v_sincospif_data);

  float32x4x2_t sc = v_sincospif_inline (x, d);

  vst1q_f32 ((float *) out_sin, sc.val[0]);
  vst1q_f32 ((float *) out_cos, sc.val[1]);
}

#if WANT_TRIGPI_TESTS
PL_TEST_ULP (_ZGVnN4v_sincospif_sin, 2.54)
PL_TEST_ULP (_ZGVnN4v_sincospif_cos, 2.68)
#  define V_SINCOSF_INTERVAL(lo, hi, n)                                       \
    PL_TEST_INTERVAL (_ZGVnN4v_sincospif_sin, lo, hi, n)                      \
    PL_TEST_INTERVAL (_ZGVnN4v_sincospif_cos, lo, hi, n)
V_SINCOSF_INTERVAL (0, 0x1p31, 500000)
V_SINCOSF_INTERVAL (-0, -0x1p31, 500000)
V_SINCOSF_INTERVAL (0x1p31, inf, 10000)
V_SINCOSF_INTERVAL (-0x1p31, -inf, 10000)
#endif
