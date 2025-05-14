// Single-precision comparison, returning a boolean in r0, true if a and b are
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
  MOVS    r2, #0xFF
  LSLS    r2, r2, #24
  LSLS    r3, r0, #1
  CMP     r3, r2
  BHI     fcmp_NaN
  LSLS    r3, r1, #1
  CMP     r3, r2
  BHI     fcmp_NaN

  // If HS, then we have no NaNs and return false.
  MOVS    r0, #0
  BX      lr

  // Otherwise, we have at least one NaN, and return true.
fcmp_NaN:
  MOVS    r0, #1
  BX      lr

  .size arm_fp_fcmp_bool_un, .-arm_fp_fcmp_bool_un
