// Single-precision comparison, returning a 3-way comparison result.
//
// Copyright (c) 1994-1998,2025, Arm Limited.
// SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception

// This function has the semantics of GNU __cmpsf2: it's a three-way compare
// which returns <0 if x<y, 0 if x==y, and >0 if x>y. If the result is
// unordered (i.e. x or y or both is NaN) then it returns >0.
//
// This also makes it suitable for use as all of __eqsf2, __nesf2, __ltsf2 or
// __lesf2.

  .syntax unified
  .text
  .p2align 2

op0 .req r0
op1 .req r1
.macro SetReturnRegister
  MOV r0, #0
  MOVHI r0, #1
  MOVLO r0, #-1
.endm

  .globl arm_fp_fcmp_gnu_cmp
  .type arm_fp_fcmp_gnu_cmp,%function
arm_fp_fcmp_gnu_cmp:
  #include "fcmp.h"

fcmp_NaN:
  MOV r0, #+1
  BX lr

  .size arm_fp_fcmp_gnu_cmp, .-arm_fp_fcmp_gnu_cmp
