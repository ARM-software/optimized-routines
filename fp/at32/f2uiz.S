// Single-precision float to uint32 conversion, rounded towards zero.
//
// Copyright (c) 1994-1998,2025, Arm Limited.
// SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception

  .syntax unified
  .text
  .p2align 2

  .globl arm_fp_f2uiz
  .type arm_fp_f2uiz,%function
arm_fp_f2uiz:

  // The fast path: deliver an answer as quickly as possible for positive
  // inputs that don't overflow, and branch out of line to handle everything
  // else (negative numbers, overflows and NaNs) more slowly.
  //
  // The basic idea is to make a bare version of the mantissa, with its leading
  // 1 bit explicit at the top of the word, and shift it right by an amount
  // derived from the exponent.

  LSRS    r1, r0, #23           // r1 = exponent, or >=256 if input is negative
  RSBS    r2, r1, #31 + 0x7f    // r2 = how much to shift the mantissa right

  // If the shift count is negative, that means the input is too big, or the
  // exponent is 0xFF (so we might have a NaN), or the sign bit was set (so the
  // input is negative). Branch out of line to handle all those cases.
  BLT     f2uiz_uncommon      // negative, infinite or NaN

  // Shift the mantissa up to the top of the word, OR in the leading 1, and
  // then shift it back down to make the result.
  //
  // We're rounding towards zero, so bits shifted off the bottom can just be
  // ignored.
  LSL     r3, r0, #8            // shift mantissa up
  ORR     r3, r3, #0x80000000   // add leading 1
  MOV     r0, r3, LSR r2        // shift down again
  BX      lr

f2uiz_uncommon:
  // We come here for positive overflows, positive infinity, NaNs, and anything
  // with the sign bit set.
  //
  // Start by testing the sign bit. Branch out of line for positive values.
  TST     r0, r0
  BPL     f2uiz_positive_invalid

  // Now we have either a negative finite value, -inf, or a NaN (with the sign
  // bit set). Conveniently, _all_ of those just return 0, so we don't have to
  // bother checking which.
  //
  // If we were reporting exceptions, then inputs in the range (-1,0] would
  // _legally_ return 0 (after rounding toward zero), whereas -1 and below
  // would raise the IEEE Invalid Operation exception.
f2uiz_return_zero:
  MOV     r0, #0
  BX      lr

f2uiz_positive_invalid:
  // Here we have a positive value causing an invalid operation exception. If
  // it's a NaN, we return zero; otherwise we return UINT_MAX.
  MOV     r1, #0xFF000000
  CMP     r1, r0, LSL #1
  BLO     f2uiz_return_zero
  MOV     r0, #0xFFFFFFFF
  BX      lr

  .size arm_fp_f2uiz, .-arm_fp_f2uiz
