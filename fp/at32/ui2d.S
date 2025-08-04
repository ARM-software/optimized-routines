// Double-precision conversion from unsigned 32-bit integers.
//
// Copyright (c) 1994-1998,2025, Arm Limited.
// SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception

#include "endian.h"

  .syntax unified
  .text
  .p2align 2

  .globl arm_fp_ui2d
  .type arm_fp_ui2d,%function
arm_fp_ui2d:

  // Shift the leading bit of the input to the top of xh. In the
  // process, we detect a zero input, and branch out of line for it.
  CLZ     r3, r0
  LSLS    xh, r0, r3
  BEQ     ui2d_zero

  // Use the shift count in r3 to calculate the output exponent.
  //
  // If r3=0 then the input value was in the range [2^31,2^32), so its exponent
  // in double precision should be 0x41e. We want to reduce that by 1 so that
  // the leading bit of the mantissa will increment it when we add it in. So
  // the exponent should be 0x41d minus r3.
  RSB     r3, r3, #0x1d         // 0x1d minus shift count
  ADD     r3, r3, #0x400        // 0x41d minus shift count
  LSL     r2, r3, #20           // r2 is now shifted exponent

  // Shift the mantissa left and right to get the parts that go in xl and xh,
  // and combine with the exponent we just computed.
  LSL     xl, xh, #21           // low bits of mantissa
  ADD     xh, r2, xh, LSR #11   // high bits of mantissa + exponent
  BX      lr

ui2d_zero:
  // We come here if the input was zero. We've just set xh to 0, so we
  // only need to set xl to 0 too and return.
  MOV     xl, #0
  BX      lr

  .size arm_fp_ui2d, .-arm_fp_ui2d
