/*
 * Double-precision SVE sincospi(x, *y, *z) function.
 *
 * Copyright (c) 2024-2025, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "sv_math.h"
#include "sv_sincospi_common.h"
#include "test_defs.h"

/* Double-precision vector function allowing calculation of both sinpi and
   cospi in one function call, using shared argument reduction and polynomials.
    Worst-case error for sin is 3.09 ULP:
    _ZGVsMxvl8l8_sincospi_sin(0x1.7a41deb4b21e1p+14) got 0x1.fd54d0b327cf1p-1
						    want 0x1.fd54d0b327cf4p-1.
   Worst-case error for sin is 3.16 ULP:
    _ZGVsMxvl8l8_sincospi_cos(-0x1.11e3c7e284adep-5) got 0x1.fd2da484ff3ffp-1
						    want 0x1.fd2da484ff402p-1.
 */
void
_ZGVsMxvl8l8_sincospi (svfloat64_t x, double *out_sin, double *out_cos,
		       svbool_t pg)
{
  svfloat64x2_t sc = sv_sincospi_inline (pg, x);

  svst1 (pg, out_sin, svget2 (sc, 0));
  svst1 (pg, out_cos, svget2 (sc, 1));
}

#if WANT_C23_TESTS
TEST_ULP (_ZGVsMxvl8l8_sincospi_sin, 2.59)
TEST_ULP (_ZGVsMxvl8l8_sincospi_cos, 2.66)
#  define SV_SINCOSPI_INTERVAL(lo, hi, n)                                     \
    TEST_SYM_INTERVAL (_ZGVsMxvl8l8_sincospi_sin, lo, hi, n)                  \
    TEST_SYM_INTERVAL (_ZGVsMxvl8l8_sincospi_cos, lo, hi, n)
SV_SINCOSPI_INTERVAL (0, 0x1p-63, 10000)
SV_SINCOSPI_INTERVAL (0x1p-63, 0.5, 50000)
SV_SINCOSPI_INTERVAL (0.5, 0x1p53, 50000)
SV_SINCOSPI_INTERVAL (0x1p53, inf, 10000)
#endif
CLOSE_SVE_ATTR
