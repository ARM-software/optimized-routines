// Single-precision comparison, returning a boolean in r0, true if x and y are
// 'unordered', i.e. one or both of them is a NaN.
//
// Copyright (c) 1994-1998,2025, Arm Limited.
// SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception

  .syntax unified
  .text
  .p2align 2

  .globl arm_fp_fcmp_bool_un
  .type arm_fp_fcmp_bool_un,%function
arm_fp_fcmp_bool_un:

  // This function isn't based on the general-purpose code in fcmp.h, because
  // it's more effort than needed. Here we just need to identify whether or not
  // there's at least one NaN in the inputs. There's no need to vary that check
  // based on the sign bit, so we might as well just do the NaN test as quickly
  // as possible.
  MOV     r12, #0xFF << 24
  CMP     r12, r0, LSL #1    // if LO, then r12 < (r0 << 1), so r0 is a NaN
  CMPHS   r12, r1, LSL #1    // if not LO, then do the same check for r1

  // If HS, then we have no NaNs and return false. We do this as quickly as we
  // can (not stopping to take two instructions setting up r0 for both
  // possibilities), on the assumption that NaNs are rare and we want to
  // optimize for the non-NaN path.
  MOVHS   r0, #0
  BXHS    lr

  // Otherwise, we have at least one NaN, and return true.
  MOV     r0, #1
  BX      lr

  .size arm_fp_fcmp_bool_un, .-arm_fp_fcmp_bool_un
