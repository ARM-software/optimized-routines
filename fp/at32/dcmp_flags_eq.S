// Double-precision comparison, returning EQ / NE in the flags.
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

  .globl arm_fp_dcmp_flags_eq
  .type arm_fp_dcmp_flags_eq,%function
arm_fp_dcmp_flags_eq:
  #include "dcmp.h"

dcmp_NaN:
  // If any NaN is involved, the return value for this comparison is false.
  MOVS r0, #1   // the value in r0 is unimportant; this just sets Z=0
  BX lr

  .size arm_fp_dcmp_flags_eq, .-arm_fp_dcmp_flags_eq
