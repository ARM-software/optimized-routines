// Single-precision conversion from signed 32-bit integers.
//
// Copyright (c) 1994-1998,2025, Arm Limited.
// SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception

  .syntax unified
  .text
  .p2align 2

  .globl arm_fp_i2f
  .type arm_fp_i2f,%function
arm_fp_i2f:

  // Isolate the input integer's sign bit in r1, and if the input is negative,
  // negate it.
  ANDS    r1, r0, #0x80000000
  RSBMI   r0, r0, #0

  // Find the highest set bit of the input, and shift it up to the top bit. r3
  // contains the amount we shifted by, and r2 the shifted value.
  CLZ     r3, r0
  LSLS    r2, r0, r3

  // Convert the shift distance into the exponent of the output float. The
  // exponent for an integer with bit 31 set should be 0x7f (the IEEE exponent
  // bias) plus 31, which is 0x9e. Here we reduce that by 1, because when we
  // add the mantissa to it, the leading mantissa bit will increment it.
  RSB     r3, r3, #0x9d

  // A side effect of the LSLS above was to set the Z flag if the input integer
  // was actually zero. In that situation, we can just return immediately,
  // because r0 still _contains_ the input integer, which has the same
  // representation as the floating-point +0 that we should return.
#if !__thumb__
  BXEQ    lr
#else
  // In Thumb, we do the conditional return by branching to a return
  // instruction later in the function. This costs more time in the case where
  // the return is taken, but saves an IT in the case where it's not, and we
  // assume that nonzero integers are converted to FP more often than zero is.
  // (This also improves the _worst-case_ running time, because the nonzero
  // code path is the limiting factor.)
  BEQ     i2f_return
#endif

  // Shift the exponent up to its final bit position, and combine it with the
  // sign.
  ORRS    r1, r1, r3, LSL #23

  // Recombine the mantissa with the sign+exponent, and round. This is done
  // differently between Arm and Thumb.
#if !__thumb__
  // Arm rounding sequence: shift the round bit off the top of r2 into C, and
  // simultaneously set Z if the lower-down bits are all zero.
  LSLS    r3, r2, #25
  // Recombine mantissa with sign+exponent, using ADC so that this also adds 1
  // if we're rounding up.
  ADC     r0, r1, r2, LSR #8
  // If C was clear, we can't possibly need to round to even, so return.
  BXCC    lr
  // Now we've definitely rounded up, and if Z is set, round to even.
  BICEQ   r0, r0, #1
  BX      lr
#else
  // Thumb rounding sequence: we do things in a slightly different order, by
  // recombining first with plain ADD, and _then_ testing the round bit. On
  // simple M-profile CPUs like Cortex-M3, this avoids the IT instruction
  // (inserted before BXCC lr) costing a cycle, because it immediately follows
  // a 16-bit LSLS instruction, so the CPU had already fetched it.
  //
  // So we save a cycle in the case where we don't round up, at the cost of a
  // cycle in the case where we do (requiring a separate ADD instruction after
  // the BXCC lr isn't taken). We expect that this is a good trade, on the
  // theory that _most_ integers converted into floating point are not large
  // enough to need rounding at all, so all the exact cases _and_ half the
  // inexact ones will benefit from the saving.
  ADD     r0, r1, r2, LSR #8    // r0 is now sign+exp+mant, unrounded
#if __thumb__
  LSLS.N  r3, r2, #25           // .N to make sure it's assembled as 16-bit
#else
  LSLS    r3, r2, #25           // GNU assembler treats .N as an error in Arm
#endif
  BXCC    lr
  // Now if we didn't take the return, we must definitely round up, and
  // conditionally round to even.
  ADD     r0, r0, #1
  BICEQ   r0, r0, #1
i2f_return:                     // label we branch to from the 0 case above
  BX      lr
#endif

  .size arm_fp_i2f, .-arm_fp_i2f
