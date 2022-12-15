/*
 * PL macros to aid testing. This version of this file is used for building the
 * routine, not the tests. Separate definitions are found in test/pl_test.h
 * which emit test parameters.
 *
 * Copyright (c) 2022, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception.
 */

/* Emit max ULP threshold - silenced for building the routine.  */
#define PL_TEST_ULP(f, l)

/* Emit alias. The PL_TEST_ALIAS declaration is piggy-backed on top of
   strong_alias. Use PL_ALIAS instead of strong_alias to make sure the alias is
   also added to the test suite.  */
#define PL_ALIAS(a, b) strong_alias (a, b)
