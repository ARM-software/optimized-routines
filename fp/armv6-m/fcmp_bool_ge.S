// Single-precision comparison, returning a boolean in r0, true if a >= b.
//
// Copyright (c) 1994-1998,2025, Arm Limited.
// SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception

  .syntax unified
  .text
  .p2align 2

op0 .req r0
op1 .req r1
.macro SetReturnRegister
  BHS 0f
  MOVS r0, #0
  BX lr
0:
  MOVS r0, #1
  BX lr
.endm

  .globl arm_fp_fcmp_bool_ge
  .type arm_fp_fcmp_bool_ge,%function
arm_fp_fcmp_bool_ge:
  #include "fcmp.h"

fcmp_NaN:
  // If any NaN is involved, the return value for this comparison is false.
  MOVS r0, #0
  BX lr

  .size arm_fp_fcmp_bool_ge, .-arm_fp_fcmp_bool_ge
