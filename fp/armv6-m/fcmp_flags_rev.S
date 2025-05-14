// Single-precision comparison, with the input operands reversed, returning
// LO / EQ / HI in the flags, with the 'unordered result' case also returning
// HI.
//
// Copyright (c) 1994-1998,2025, Arm Limited.
// SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception

  .syntax unified
  .text
  .p2align 2

op0 .req r1
op1 .req r0
.macro SetReturnRegister
.endm

  .globl arm_fp_fcmp_flags_rev
  .type arm_fp_fcmp_flags_rev,%function
arm_fp_fcmp_flags_rev:
  #include "fcmp.h"

fcmp_NaN:
  // If any NaN is involved, the return value for this comparison is
  // to set the flags as if a > b, so that the HI condition will return true.
  MOVS r0, #1
  CMP r0, #0
  BX lr

  .size arm_fp_fcmp_flags_rev, .-arm_fp_fcmp_flags_rev
