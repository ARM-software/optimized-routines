// Double-precision comparison, returning a boolean in r0, true if x == y.
//
// Copyright (c) 1994-1998,2025, Arm Limited.
// SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception

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
  MOVEQ r0, #1
.endm
.macro SetReturnRegisterNE
  MOVNE r0, #0
.endm

  .globl arm_fp_dcmp_bool_eq
  .type arm_fp_dcmp_bool_eq,%function
arm_fp_dcmp_bool_eq:
  #include "dcmp.h"

dcmp_NaN:
  // If any NaN is involved, the return value for this comparison is false.
  MOV r0, #0
  BX lr

  .size arm_fp_dcmp_bool_eq, .-arm_fp_dcmp_bool_eq
