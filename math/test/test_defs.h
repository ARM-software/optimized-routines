/*
 * Helper macros for emitting various details about routines for consumption by
 * runulp.sh.
 *
 * Copyright (c) 2025, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception.
 */

#define TEST_ULP(f, l) TEST_ULP f l
#define TEST_ULP_NONNEAREST(f, l) TEST_ULP_NONNEAREST f l

#define TEST_INTERVAL(f, lo, hi, n) TEST_INTERVAL f lo hi n
#define TEST_SYM_INTERVAL(f, lo, hi, n)                                       \
  TEST_INTERVAL (f, lo, hi, n)                                                \
  TEST_INTERVAL (f, -lo, -hi, n)
// clang-format off
#define TEST_INTERVAL2(f, xlo, xhi, ylo, yhi, n)                            \
  TEST_INTERVAL f xlo,ylo xhi,yhi n
// clang-format on

#define TEST_CONTROL_VALUE(f, c) TEST_CONTROL_VALUE f c
