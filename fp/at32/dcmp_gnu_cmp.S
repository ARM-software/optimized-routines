// Double-precision comparison, returning a 3-way comparison result.
//
// Copyright (c) 1994-1998,2025, Arm Limited.
// SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception

// This function has the semantics of GNU __cmpdf2: it's a three-way compare
// which returns <0 if x<y, 0 if x==y, and >0 if x>y. If the result is
// unordered (i.e. x or y or both is NaN) then it returns >0.
//
// This also makes it suitable for use as all of __eqdf2, __nedf2, __ltdf2 or
// __ledf2.

  .syntax unified
  .text
  .p2align 2

#include "endian.h"

op0h .req xh
op0l .req xl
op1h .req yh
op1l .req yl
.macro SetReturnRegister
  MOV r0, #0
  MOVHI r0, #1
  MOVLO r0, #-1
.endm
.macro SetReturnRegisterNE
  MOVNE r0, #-1
  MOVHI r0, #1
.endm

  .globl arm_fp_dcmp_gnu_cmp
  .type arm_fp_dcmp_gnu_cmp,%function
arm_fp_dcmp_gnu_cmp:
  #include "dcmp.h"

dcmp_NaN:
  MOV r0, #+1
  BX lr

  .size arm_fp_dcmp_gnu_cmp, .-arm_fp_dcmp_gnu_cmp
