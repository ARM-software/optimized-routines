/*
 * PL macros for emitting various details about routines for consumption by
 * runulp.sh.
 *
 * Copyright (c) 2022-2024, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception.
 */

/* Emit the max ULP threshold, l, for routine f.  */
#define TEST_ULP(f, l) TEST_ULP f l

/* Emit routine name if e == 0 and f is expected to correctly trigger fenv
   exceptions. e allows declaration to be emitted conditionally on
   WANT_SIMD_EXCEPT - defer expansion by one pass to allow those flags to be
   expanded properly.  */
#define TEST_DISABLE_FENV(f) TEST_DISABLE_FENV f
#define TEST_DISABLE_FENV_IF_NOT(f, e) TEST_DISABLE_FENV_IF_NOT_ (f, e)
#define TEST_DISABLE_FENV_IF_NOT_(f, e) TEST_DISABLE_FENV_IF_NOT_##e (f)
#define TEST_DISABLE_FENV_IF_NOT_0(f) TEST_DISABLE_FENV (f)
#define TEST_DISABLE_FENV_IF_NOT_1(f)

#define TEST_INTERVAL(f, lo, hi, n) TEST_INTERVAL f lo hi n
#define TEST_SYM_INTERVAL(f, lo, hi, n)                                       \
  TEST_INTERVAL (f, lo, hi, n)                                                \
  TEST_INTERVAL (f, -lo, -hi, n)
// clang-format off
#define TEST_INTERVAL2(f, xlo, xhi, ylo, yhi, n)                            \
  TEST_INTERVAL f xlo,ylo xhi,yhi n

#define TEST_CONTROL_VALUE(f, c) TEST_CONTROL_VALUE f c
// clang-format on
