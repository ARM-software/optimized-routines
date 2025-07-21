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
  ADCS    ah, ah, ah            // shift ah left, bringing in the C bit
  MOVS    al, #1                // now al is free, make the test constant
  LSLS    al, al, #21           //   by shifting 1 left to make 0x00200000
  CMN     ah, al                // HI if a is a NaN
  BHI     dcmp_NaN
  CMP     bl, #1                // set C if bl is nonzero
  ADCS    bh, bh, bh            // shift bh left, bringing in the C bit
  CMN     bh, al                // HI if b is a NaN
  BHI     dcmp_NaN

  MOVS    r0, #0
  BX      lr

dcmp_NaN:
  MOVS    r0, #1
  BX      lr

  .size arm_fp_dcmp_bool_un, .-arm_fp_dcmp_bool_un
