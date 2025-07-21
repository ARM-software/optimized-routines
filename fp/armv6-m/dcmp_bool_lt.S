// Double-precision comparison, returning a boolean in r0, true if a < b.
//
// Copyright (c) 1994-1998,2025, Arm Limited.
// SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception

  .syntax unified
  .text
  .p2align 2

#include "endian.h"

op0h .req ah
op0l .req al
op1h .req bh
op1l .req bl
.macro SetReturnRegister
  BLO     0f
  MOVS    r0, #0
  // This macro is always called immediately before returning from the
  // function, so it's safe to use the same return instruction here, instead of
  // wasting time branching forward to the end of the macro.
  POP     {r4,r5,r6,pc}
0:
  MOVS    r0, #1
.endm

  .globl arm_fp_dcmp_bool_lt
  .type arm_fp_dcmp_bool_lt,%function
arm_fp_dcmp_bool_lt:
  #include "dcmp.h"

dcmp_NaN:
  // If any NaN is involved, the return value for this comparison is false.
  MOVS    r0, #0
  POP     {r4,r5,r6,pc}

  .size arm_fp_dcmp_bool_lt, .-arm_fp_dcmp_bool_lt
