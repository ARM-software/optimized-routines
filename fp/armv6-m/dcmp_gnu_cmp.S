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
  BHI 0f
  BLO 1f
  MOVS r0, #0
  // This macro is always called immediately before returning from the
  // function, so it's safe to use the same return instruction here, instead of
  // wasting time branching forward to the end of the macro.
  POP     {r4,r5,r6,pc}
0:
  MOVS r0, #1
  POP     {r4,r5,r6,pc}
1:
  MOVS r0, #1
  RSBS r0, r0, #0
  POP     {r4,r5,r6,pc}
.endm

  .globl arm_fp_dcmp_gnu_cmp
  .type arm_fp_dcmp_gnu_cmp,%function
arm_fp_dcmp_gnu_cmp:
  #include "dcmp.h"

dcmp_NaN:
  MOVS r0, #1
  POP     {r4,r5,r6,pc}

  .size arm_fp_dcmp_gnu_cmp, .-arm_fp_dcmp_gnu_cmp
