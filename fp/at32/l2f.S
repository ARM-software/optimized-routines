// Single-precision conversion from signed and unsigned 64-bit integers.
//
// Copyright (c) 1994-1998,2025, Arm Limited.
// SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception

#include "endian.h"

  .syntax unified
  .text
  .p2align 2

  .globl arm_fp_l2f
  .type arm_fp_l2f,%function
arm_fp_l2f:

  // Isolate the input integer's sign bit in r2, and if the input is negative,
  // negate it.
  ANDS    r2, ah, #0x80000000
  BPL     0f                    // if positive, skip the negation
  RSBS    al, al, #0
#if !__thumb__
  RSC     ah, ah, #0
#else
  // Thumb has no RSC, so simulate it by handling the carry flag afterwards
  RSB     ah, ah, #0
  SUBCC   ah, ah, #1
#endif
0:

  // Combine the sign in r2 with the FP exponent of 1. So r2 now holds the
  // single-precision encoding of +1 or -1 as appropriate.
  ORR     r2, r2, #0x3f800000

  // Now we have a positive 64-bit integer in ah,al, and a starting sign +
  // exponent in r2.
  //
  // We also come here from the unsigned-integer conversion function below, so
  // we must handle ah,al having any possible values at all, even 2^63 or
  // greater.
l2f_normalise:

  // Add 30 to the exponent in r2, so that it holds +2^30 or -2^30. The idea is
  // that after we normalise the input integer into a FP mantissa with the
  // topmost 1 in bit 23, adding that will increment by one more, so that this
  // exponent will be correct if the input has its high bit in bit 31. We'll
  // decrease the exponent if CLZ returns a positive value, and increment it by
  // 32 if the high word is used.
  //
  // You might ask why we didn't set up r2 to have this value in the first
  // place, by ORRing the sign bit with 0x4e800000 instead of 0x3f800000. The
  // answer is because 0x4e800000 can't be represented in the immediate field
  // of an AArch32 data-processing instruction, so we can't avoid using two
  // instructions.
  ADD     r2, r2, #30 << 23

  // Start setting up r3 to be the exponent adjustment, and set ah to be the
  // highest _nonzero_ word of the input. If ah = 0, set r3 = 0 and copy al
  // (the only nonzero input word) into ah; if ah != 0, set r3 = 32.
  MOVS    r3, ah                // sets r3=0 if ah=0, testing at the same time
  MOVNE   r3, #32               // if that didn't happen, set r3=32
  MOVSEQ  ah, al                // and otherwise, copy al into ah

  // Using a MOVS for the final copy has the side effect that we've also just
  // tested whether ah = al = 0. If so, then the entire input value was zero,
  // so we should return 0. Conveniently, that's the value in both al and ah
  // right now, so no matter which of those is r0 (which varies with
  // endianness) we can just return.
  BXEQ    lr

  // Now we know ah contains the highest set bit of the input. Find that bit,
  // shift it up to the top of the word, and adjust the shift count
  // appropriately.
  //
  // After this, r3 contains the full exponent adjustment we'll need to add to
  // the starting exponent in r2: it takes values from -31 (if the input was 1)
  // to +32 (if the input was 2^63 or bigger).
  CLZ     r12, ah
  MOV     ah, ah, LSL r12
  SUB     r3, r3, r12

  // If the input integer is < 2^32, then we've now set up ah to be the full
  // output mantissa (with its leading bit at the top of the word). If not,
  // then we still need to add some bits from al.
  //
  // We don't need to spend an instruction on deciding which: it's enough to
  // just shift al right by whatever is in r3. In the case where we don't want
  // it (because the bits in al are already in the output mantissa), r3 <= 0.
  // If r3 = 0 (the input was an exactly 32-bit integer) then the bits in al
  // will exactly overlay the ones already in ah and make no difference; if r3
  // < 0 then the AArch32 shift instruction semantics will treat it as a shift
  // of more than 32 bits, shifting al right off the bottom of the word, and
  // again not modify ah.
#if !__thumb__
  ORR     ah, ah, al, LSR r3   // if shift negative then ah unaltered
#else
  // Thumb can't fold a register-controlled shift into an ORR, so we must use
  // two separate instructions.
  LSR     r12, al, r3
  ORR     ah, ah, r12
#endif

  // Combine the exponent adjustment in r3 with the starting exponent and sign
  // in r2. These parts of the output are now ready to combine with the
  // mantissa, once we've shifted it down and rounded it.
  ADD     r2, r2, r3, LSL #23

  // Now we must round. The mantissa in r12 contains the top 32 bits of the
  // full result, including the bit we're going to shift just off the bottom
  // (which controls the basic 'round up or down?' question). So we can start
  // by checking those, which will handle most cases.

  // This shift moves the round bit off the top of ah into the carry flag, so
  // that C is set if we're rounding up. It also sets Z if all the bits below
  // that are zero, which _might_ mean we need to round to even, but only if
  // the further bits in al are also zero. But if Z is _not_ set then we can
  // return without checking al.
  LSLS    r12, ah, #25

#ifndef __BIG_ENDIAN__
  // We're about to overwrite r0 with the preliminary output. This will be our
  // last use of ah, but we still need al later. So in little-endian mode,
  // where al _is_ r0, we must spend an extra instruction on saving it.
  MOV     r12, al
#endif

  // Recombine the mantissa (shifted down to the right position) with the sign
  // and exponent in r2. Using ADC also rounds up if C is set.
  ADC     r0, r2, ah, LSR #8

  // If C was clear, we didn't round up, so we don't need to undo that by
  // rounding to even. And if Z was clear, we're not rounding to even anyway.
  // So in either case, we're done.
  BXCC    lr
  BXNE    lr

  // The slow path: nothing in the top 32 bits of the mantissa ruled out having
  // to round to even. Now we must check the rest of the mantissa bits in al.
  //
  // This RSB instruction converts the previous exponent adjustment value (-31
  // for smallest integer, +32 for largest) into a value from 0 (_largest_
  // integer) to 63 (smallest). So if the integer occupied n bits of ah, then
  // 32-n bits of al ended up in the initial mantissa word, so shifting al left
  // by 32-n will catch precisely the bits of al that didn't. And if the
  // integer was entirely in al, then this shift count will be >=32, so the
  // left shift will throw away all of al.
  RSB     r3, r3, #32

  // Shift al to include just the shifted-off bits, setting Z if they're all
  // zero. Then we know whether to round to even by clearing bit 0 of the
  // output.
#ifdef __BIG_ENDIAN__
  LSLS    r12, al, r3           // the low word is still in al itself
#else
  LSLS    r12, r12, r3          // we moved it into r12 earlier
#endif
  BICEQ   r0, r0, #1

  // And whether we did that or not, we're finished.
  BX      lr

  .size arm_fp_l2f, .-arm_fp_l2f

  .globl arm_fp_ul2f
  .type arm_fp_ul2f,%function
arm_fp_ul2f:
  // Jump to l2f_normalise above, without negating the input, and having set up
  // r2 unconditionally to indicate that a positive output is wanted.
  MOV     r2, #0x3f800000
  B       l2f_normalise

  .size arm_fp_ul2f, .-arm_fp_ul2f
