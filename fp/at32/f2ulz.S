// Single-precision float to uint64 conversion, rounded towards zero.
//
// Copyright (c) 1994-1998,2025, Arm Limited.
// SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception

#include "endian.h"

  .syntax unified
  .text
  .p2align 2

  .globl arm_fp_f2ulz
  .type arm_fp_f2ulz,%function
arm_fp_f2ulz:

  // The fast path: deliver an answer as quickly as possible for positive
  // inputs that don't overflow, and branch out of line to handle everything
  // else (negative numbers, overflows and NaNs) more slowly.
  //
  // The basic idea is to make a bare version of the mantissa, with its leading
  // 1 bit explicit at the top of the word, and shift it right by an amount
  // derived from the exponent.

  // Shift the exponent down to the bottom of the word.
  LSRS    r2, r0, #23           // r2 = exponent, or >=256 if input is negative
  RSBS    r2, r2, #63 + 0x7f    // r2 = how much to shift the mantissa right

  // If the shift count is negative, that means the input is too big, or the
  // exponent is 0xFF (so we might have a NaN), or the sign bit was set (so the
  // input is negative). Branch out of line to handle all those cases.
  BLO     f2ulz_uncommon

  // Shift the mantissa to the top of the word, and put on the leading 1 bit.
  LSL     r3, r0, #8
  ORR     r3, r3, #0x80000000

  // Make the top word of the result, which is the easy part: if the shift
  // count is too big, nothing goes wrong, we just end up with whatever part of
  // the mantissa remained in this word.
  MOV     ah, r3, LSR r2

  // Make the bottom word of the result. This might involve shifting the
  // mantissa either left or right, depending on the exponent.
  //
  // We're rounding towards zero, so bits shifted off the bottom can just be
  // ignored.
  SUBS    r12, r2, #32          // r12 = how far to shift mantissa down
  MOVHS   al, r3, LSR r12       // if that's positive, just do it
  RSBLO   r12, r12, #0          // otherwise, negate it
  MOVLO   al, r3, LSL r12       // and shift left by that much instead
  BX      lr

f2ulz_uncommon:
  // We come here for positive overflows, positive infinity, NaNs, and anything
  // with the sign bit set.
  //
  // Start by testing the sign bit. Branch out of line for positive values.
  TST     r0, r0
  BPL     f2ulz_positive_invalid

  // Now we have either a negative finite value, -inf, or a NaN (with the sign
  // bit set). Conveniently, _all_ of those just return 0, so we don't have to
  // bother checking which.
  //
  // If we were reporting exceptions, then inputs in the range (-1,0] would
  // _legally_ return 0 (after rounding toward zero), whereas -1 and below
  // would raise the IEEE Invalid Operation exception.
f2ulz_return_zero:
  MOV     al, #0
  MOV     ah, #0
  BX      lr

f2ulz_positive_invalid:
  // Here we have a positive value causing an invalid operation exception. If
  // it's a NaN, we return zero; otherwise we return UINT_MAX.
  MOV     r1, #0xFF000000
  CMP     r1, r0, LSL #1
  BLO     f2ulz_return_zero
  MOV     al, #0xFFFFFFFF
  MOV     ah, #0xFFFFFFFF
  BX      lr

  .size arm_fp_f2ulz, .-arm_fp_f2ulz
