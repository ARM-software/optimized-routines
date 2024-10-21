/*
 * PL macros for emitting various details about routines for consumption by
 * runulp.sh.
 *
 * Copyright (c) 2022-2024, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception.
 */

/* Emit the max ULP threshold, l, for routine f.  */
#define PL_TEST_ULP(f, l) PL_TEST_ULP f l

/* Emit routine name if e == 0 and f is expected to correctly trigger fenv
   exceptions. e allows declaration to be emitted conditionally on
   WANT_SIMD_EXCEPT - defer expansion by one pass to allow those flags to be
   expanded properly.  */
#define PL_TEST_DISABLE_FENV(f) PL_TEST_DISABLE_FENV f
#define PL_TEST_DISABLE_FENV_IF_NOT(f, e) PL_TEST_DISABLE_FENV_IF_NOT_ (f, e)
#define PL_TEST_DISABLE_FENV_IF_NOT_(f, e) PL_TEST_DISABLE_FENV_IF_NOT_##e (f)
#define PL_TEST_DISABLE_FENV_IF_NOT_0(f) PL_TEST_DISABLE_FENV (f)
#define PL_TEST_DISABLE_FENV_IF_NOT_1(f)

#define PL_TEST_INTERVAL(f, lo, hi, n) PL_TEST_INTERVAL f lo hi n
#define PL_TEST_SYM_INTERVAL(f, lo, hi, n)                                    \
  PL_TEST_INTERVAL (f, lo, hi, n)                                             \
  PL_TEST_INTERVAL (f, -lo, -hi, n)
#define PL_TEST_INTERVAL_C(f, lo, hi, n, c) PL_TEST_INTERVAL f lo hi n c
#define PL_TEST_SYM_INTERVAL_C(f, lo, hi, n, c)                               \
  PL_TEST_INTERVAL_C (f, lo, hi, n, c)                                        \
  PL_TEST_INTERVAL_C (f, -lo, -hi, n, c)
// clang-format off
#define PL_TEST_INTERVAL2(f, xlo, xhi, ylo, yhi, n)                            \
  PL_TEST_INTERVAL f xlo,ylo xhi,yhi n
// clang-format on
