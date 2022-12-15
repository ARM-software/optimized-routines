/*
 * PL macros for emitting various details about routines for consumption by
 * runulp.sh.
 *
 * Copyright (c) 2022, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception.
 */

/* Emit the max ULP threshold, l, for routine f.  */
#define PL_TEST_ULP(f, l) PL_TEST_ULP f l

/* Emit aliases to allow test params to be mapped from aliases back to their
   aliasees.  */
#define PL_ALIAS(a, b) PL_TEST_ALIAS a b
