// Single-precision multiplication.
//
// Copyright (c) 2009,2010,2025, Arm Limited.
// SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception

  .syntax unified
  .text
  .thumb
  .p2align 2

  .globl arm_fp_fmul
  .type arm_fp_fmul,%function
arm_fp_fmul:
  PUSH {r4,r5,r6,lr}

  // Get exponents of the inputs, and check for uncommon values. In the process
  // of this we also compute the sign, because it's marginally quicker that
  // way.
  LSLS    r2, r0, #1
  ADCS    r4, r4, r4    // set r4[0] to sign bit of a
  LSLS    r3, r1, #1
  ADCS    r4, r4, r3    // set r4[0] to the output sign
  LSRS    r2, r2, #24
  BEQ     zerodenorm0   // still do the next LSRS
  LSRS    r3, r3, #24
  BEQ     zerodenorm
  CMP     r2, #255
  BEQ     naninf
  CMP     r3, #255
  BEQ     naninf
  // Compute the output exponent. We'll be generating our product _without_ the
  // leading bit, so we subtract 0x7f rather than 0x80.
  ADDS    r2, r2, r3
  SUBS    r2, r2, #0x7f
  // Blank off everything above the mantissas.
  LSLS    r0, r0, #9
  LSLS    r1, r1, #9
normalised: // we may come back here from zerodenorm
  LSRS    r0, r0, #9
  LSRS    r1, r1, #9
  // Multiply. r0 and r1 are the mantissas of the inputs but without their
  // leading bits, so the product we want in principle is P=(r0+2^23)(r1+2^23).
  // P is at most (2^24-1)^2 < 2^48, so it fits in a word and a half.
  //
  // The technique below will actually compute P - 2^46, by not adding on the
  // term where the two 2^23 are multiplied. The 48-bit result will be
  // delivered in two output registers, one containing its bottom 32 bits and
  // the other containing the top 32, so they overlap in the middle 16 bits.
  // This is done using only two multiply instructions and some bookkeeping.
  //
  // In the comments I'll write A and B for the original input mantissas (again
  // without their leading bits). I'll also decompose them as A = ah + al and
  // B = bh + bl, where al and bl are in the range 0..2^8-1 and ah,bh are
  // multiples of 2^8.
  ADDS    r5, r0, r1
  LSLS    r5, r5, #7    // r5 = (A+B) << 7
  MOVS    r6, r0
  MULS    r6, r1, r6    // r6 is congruent mod 2^32 to A*B
  LSRS    r0, r0, #8
  LSRS    r1, r1, #8
  MULS    r0, r1, r0
  LSLS    r1, r0, #16   // r1 is congruent mod 2^32 to ah*bh
  SUBS    r3, r6, r1    // now r3 is congruent mod 2^32 to
                        //   (A*B) - (ah*bh) = ah*bl + al*bh + al*bl
                        //   and hence, since that is at most 0xfeff0001,
                        //   is _exactly_ equal to that
  ADDS    r0, r0, r5    // r0 is now (ah*bh + (A+B)<<23) >> 16
  LSRS    r1, r3, #16   // r1 is the top 16 bits of r3, i.e.
                        //   (ah*bl + al*bh + al*bl) >> 16
  ADDS    r3, r0, r1    // now r3 equals
                        //   (ah*bh + ah*bl + al*bh + al*bl + (A+B)<<23) >> 16
                        //   i.e. (A*B + (A+B)<<23) >> 16,
                        //   i.e. (the right answer) >> 16.
                        // Meanwhile, r6 is exactly the bottom 32 bits of the
                        // right answer.
  // Renormalise if necessary.
  LSRS    r1, r3, #30
  BEQ     norenorm
  // Here we have to do something fiddly. Renormalisation would be a trivial
  // job if we had the leading mantissa bit - just note that it's one bit
  // position above where it should be, and shift right by one. But without
  // that bit, we currently have (2x - 2^30), and we want (x - 2^30); just
  // shifting right would of course give us (x - 2^29), so we must subtract an
  // extra 2^29 to fix this up.
  LSRS    r3, r3, #1
  MOVS    r1, #1
  LSLS    r1, r1, #29
  SUBS    r3, r3, r1
  ADDS    r2, r2, #1
norenorm:
  // Round and shift down to the right bit position.
  LSRS    r0, r3, #7    // round bit goes into the carry flag
  BCC     rounded
  ADDS    r0, r0, #1
  // In the round-up branch, we must also check if we have to round to even, by
  // testing all the bits below the round bit. We will normally not expect to,
  // so we do RTE by branching out of line and back again to avoid spending a
  // branch in the common case.
  LSLS    r5, r3, #32-7+1  // check the bits shifted out of r3 above
  BNE     rounded          // if any is nonzero, we're not rounding to even
  LSLS    r5, r6, #15      // check the bottom 17 bits of the low-order 32
                           //   (enough to overlap r3 even if we renormalised)
  BEQ     rte              // if any is nonzero, fall through, else RTE
rounded:
  // Put on the sign and exponent, check for underflow and overflow, and
  // return.
  //
  // Underflow occurs iff r2 (the output exponent) <= 0. Overflow occurs if
  // it's >= 0xFF. (Also if it's 0xFE and we rounded up to overflow, but since
  // this code doesn't report exceptions, we can ignore this case because it'll
  // happen to return the right answer regardless). So we handle most of this
  // via an unsigned comparison against 0xFF, which leaves the one case of a
  // zero exponent that we have to filter separately by testing the Z flag
  // after we shift the exponent back up into place.
  CMP     r2, #0xFF    // check for most over/underflows
  BHS     outflow      // ... and branch out of line for them
  LSLS    r5, r2, #23  // shift the exponent into its output location
  BEQ     outflow      // ... and branch again if it was 0
  LSLS    r4, r4, #31  // shift the output sign into place
  ORRS    r0, r0, r4   // and OR it in to the output
  ADDS    r0, r0, r5   // OR in the mantissa
  POP     {r4,r5,r6,pc} // and return

rte:
  // Out-of-line handler for the round-to-even case. Clear the low mantissa bit
  // and go back to the post-rounding code.
  MOVS    r5, #1
  BICS    r0, r0, r5
  B       rounded

outflow:
  CMP     r2, #0
  BGT     overflow
  // To handle underflow, we construct an intermediate value in the IEEE 754
  // style (using our existing full-length mantissa, and bias the exponent by
  // +0xC0), and indicate whether that intermediate was rounded up, down or not
  // at all. Then call the helper function __funder, which will denormalise and
  // re-round correctly.
  LSLS    r1, r0, #7    // shift up the post-rounding mantissa
  SUBS    r1, r3, r1    //   and subtract it from the pre-rounding version
  LSLS    r6, r6, #15
  CMP     r6, #1        // if the rest of the low bits are nonzero
  ADCS    r1, r1, r1    //   then set an extra bit at the bottom

  LSLS    r4, r4, #31
  ORRS    r0, r0, r4    // put on the sign
  ADDS    r2, r2, #192  // bias the exponent
  LSLS    r3, r2, #23
  ADDS    r0, r0, r3    // put on the biased exponent

  BL      __funder
  POP     {r4,r5,r6,pc}

overflow:
  // Handle overflow by returning an infinity of the correct sign.
  LSLS    r4, r4, #8    // move the sign up to bit 8
  MOVS    r0, #0xff
  ORRS    r0, r0, r4    // fill in an exponent just below it
  LSLS    r0, r0, #23   // and shift those 9 bits up to the top of the word
  POP     {r4,r5,r6,pc}

  // We come here if there's at least one zero or denormal. On the fast path
  // above, it was convenient to check these before checking NaNs and
  // infinities, but NaNs take precedence, so now we're off the fast path, we
  // must still check for those.
  //
  // At the main entry point 'zerodenorm' we want r2 and r3 to be the two input
  // exponents. So if we branched after shifting-and-checking r2, we come to
  // this earlier entry point 'zerodenorm0' so that we still shift r3.
zerodenorm0:
  LSRS    r3, r3, #24
zerodenorm:
  CMP     r2, #255
  BEQ     naninf
  CMP     r3, #255
  BEQ     naninf
  // Now we know we have at least one zero or denormal, and no NaN or infinity.
  // Check if either input is actually zero. We've ruled out 0 * infinity by
  // this point, so any zero input means we return zero of the correct sign.
  LSLS    r6, r0, #1        // is one input zero?
  BEQ     zero              // yes, go and return zero
  LSLS    r6, r1, #1        // is the other one zero?
  BNE     denorm            // if not, one must have been a denormal
zero:
  LSLS    r0, r4, #31    // shift up the output sign to make the return value
  POP     {r4,r5,r6,pc}

  // Handle denormals via the helper function __fnorm2, which will break both
  // inputs up into mantissa and exponent, renormalising and generating a
  // negative exponent if necessary.
denorm:
  PUSH    {r0,r1,r2,r3}
  MOV     r0, sp
  BL      __fnorm2
  POP     {r0,r1,r2,r3}
  // Convert __fnorm2's return values into the right form to rejoin the main
  // code path.
  LSLS    r0, r0, #1
  LSLS    r1, r1, #1
  ADDS    r2, r2, r3
  SUBS    r2, r2, #0x7f
  B       normalised

  // We come here if at least one input is a NaN or infinity. There may still
  // be zeroes (or denormals, though they make no difference at this stage).
naninf:
  MOVS    r6, #0xff
  LSLS    r6, r6, #24
  LSLS    r5, r0, #1
  CMP     r5, r6
  BHI     nan              // first operand is a NaN
  LSLS    r5, r1, #1
  CMP     r5, r6
  BHI     nan              // second operand is a NaN

  // We know we have at least one infinity, and no NaNs. We might also have a
  // zero, in which case we return the default quiet NaN.
  LSLS    r6, r0, #1
  BEQ     infzero          // if r0 is a zero, r1 must be inf
  LSLS    r6, r1, #1
  BEQ     infzero          // if r1 is a zero, r0 must be inf
  // Otherwise we have infinity * infinity, or infinity * finite. Just return
  // an appropriately signed infinity.
  B       overflow         // reuse the code there

  // We come here if at least one input is a NaN. Hand off to __fnan2, which
  // propagates an appropriate NaN to the output, dealing with the special
  // cases of signalling/quiet NaNs.
nan:
  BL      __fnan2
  POP     {r4,r5,r6,pc}

  // Return a quiet NaN as the result of infinity * zero.
infzero:
  LDR     r0, =0x7fc00000
  POP     {r4,r5,r6,pc}

  .size arm_fp_fmul, .-arm_fp_fmul
