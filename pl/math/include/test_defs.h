/*
 * PL macros to aid testing. This version of this file is used for building the
 * routine, not the tests. Separate definitions are found in test/test_defs.h
 * which emit test parameters.
 *
 * Copyright (c) 2022-2024, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception.
 */

/* Emit max ULP threshold - silenced for building the routine.  */
#define TEST_ULP(f, l)

#define TEST_DISABLE_FENV(f)
#define TEST_DISABLE_FENV_IF_NOT(f, e)

#define TEST_INTERVAL(f, lo, hi, n)
#define TEST_SYM_INTERVAL(f, lo, hi, n)
#define TEST_INTERVAL2(f, xlo, xhi, ylo, yhi, n)

#define TEST_CONTROL_VALUE(f, c)
