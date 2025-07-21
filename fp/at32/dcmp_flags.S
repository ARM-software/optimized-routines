// Double-precision comparison, returning LO / EQ / HI in the flags, with the
// 'unordered result' case also returning HI.
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
.endm
.macro SetReturnRegisterNE
.endm

  .globl arm_fp_dcmp_flags
  .type arm_fp_dcmp_flags,%function
arm_fp_dcmp_flags:
  #include "dcmp.h"

dcmp_NaN:
  // If any NaN is involved, the return value for this comparison is
  // to set the flags as if a > b, so that the HI condition will return true.
  MOV r0, #1
  CMP r0, #0
  BX lr

  .size arm_fp_dcmp_flags, .-arm_fp_dcmp_flags
