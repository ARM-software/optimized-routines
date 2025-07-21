// Double-precision comparison, returning a boolean in r0, true if a and b are
// 'unordered', i.e. one or both of them is a NaN.
//
// Copyright (c) 1994-1998,2025, Arm Limited.
// SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception

  .syntax unified
  .text
  .p2align 2

#include "endian.h"

  .globl arm_fp_dcmp_bool_un
  .type arm_fp_dcmp_bool_un,%function
arm_fp_dcmp_bool_un:

  // This function isn't based on the general-purpose code in dcmp.h, because
  // it's more effort than needed. Here we just need to identify whether or not
  // there's at least one NaN in the inputs. There's no need to vary that check
  // based on the sign bit, so we might as well just do the NaN test as quickly
  // as possible.
  //
  // We do this by shifting the high word of each input left to get rid of the
  // sign bit, shifting a bit in at the bottom which is 1 if any bit is set in
  // the low word. Then we check if the result is _greater_ than 0xFFE00000
  // (but not equal), via adding 0x00200000 to it and testing for the HI
  // condition (carry flag set, but Z clear).
  //
  // Once we've done that transformation to the first input ah:al, we
  // free up al to contain our constant 0x00200000, so there's no need
  // to push any registers.
  CMP     al, #1                // set C if al is nonzero
  ADC     ah, ah, ah            // shift ah left, bringing in the C bit
  CMP     bl, #1                // set C if bl is nonzero
  ADC     bh, bh, bh            // shift bh left, bringing in the C bit
  CMN     ah, #1 << 21          // if HI, then a is a NaN
  CMNLS   bh, #1 << 21          // if not HI, then do the same check for b

  // If LS, then we have no NaNs and return false. We do this as quickly as we
  // can (not stopping to take two instructions setting up r0 for both
  // possibilities), on the assumption that NaNs are rare and we want to
  // optimize for the non-NaN path.
  MOVLS   r0, #0
  BXLS    lr

  // Otherwise, we have at least one NaN, and return true.
  MOV     r0, #1
  BX      lr

  .size arm_fp_dcmp_bool_un, .-arm_fp_dcmp_bool_un
