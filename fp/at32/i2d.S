// Double-precision conversion from signed 32-bit integers.
//
// Copyright (c) 1994-1998,2025, Arm Limited.
// SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception

#include "endian.h"

  .syntax unified
  .text
  .p2align 2

  .globl arm_fp_i2d
  .type arm_fp_i2d,%function
arm_fp_i2d:

  // Start by separating the input into its sign and its absolute value.
  ANDS    r2, r0, #0x80000000   // get sign
  RSBMI   r0, r0, #0            // negate input if it was negative

  // Shift the leading bit of the input to the top of ah. In the
  // process, we detect a zero input, and branch out of line for it.
  CLZ     r3, r0
  LSLS    ah, r0, r3
  BEQ     i2d_zero

  // Use the shift count in r3 to calculate the output exponent, and combine it
  // with the sign bit in r2.
  //
  // If r3=0 then the input value was in the range [2^31,2^32), so its exponent
  // in double precision should be 0x41e. We want to reduce that by 1 so that
  // the leading bit of the mantissa will increment it when we add it in. So
  // the exponent should be 0x41d minus r3.
  //
  // (For a signed integer, the only possible value in that range would be
  // 2^31, obtained as the absolute value of an original input of -2^31.)
  RSB     r3, r3, #0x1d         // 0x1d minus shift count
  ADD     r3, r3, #0x400        // 0x41d minus shift count
  ADD     r2, r2, r3, LSL #20   // r2 is now sign+exponent

  // Shift the mantissa left and right to get the parts that go in al and ah,
  // and combine with the sign and exponent we just computed.
  LSL     al, ah, #21           // low bits of mantissa
  ADD     ah, r2, ah, LSR #11   // high bits of mantissa + sign + exponent
  BX      lr

i2d_zero:
  // We come here if the input was zero. We've just set ah to 0, so we
  // only need to set al to 0 too and return.
  MOV     al, #0
  BX      lr

  .size arm_fp_i2d, .-arm_fp_i2d
