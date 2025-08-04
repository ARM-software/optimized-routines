// Double-precision division.
//
// Copyright (c) 1994-1998,2025, Arm Limited.
// SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception

// The basic strategy of this division code is to use Newton-Raphson iteration
// to calculate an approximation to 1/y, then multiply it by x. This procedure
// delivers a quotient with 10 extra bits of precision, but which isn't exact.
// We know an upper bound on its possible error, which gives an interval of
// possible values for the true quotient. So we can check the 10 extra bits to
// see whether a rounding boundary lies within the interval. If not, then we
// can round and return without worrying further; otherwise, we go to slower
// correction code that multiplies the approximate quotient back up by y and
// checks it against x.
//
// This strategy depends critically on the upper bound on the approximation
// error. Underestimating the error introduces a bug; overestimating it costs
// performance, by sending more cases than necessary to the slow path. To give
// high confidence of its correctness, the upper bound is proved formally by
// Gappa. But Gappa can't directly understand Arm machine code, so we also need
// to check that the machine code and the Gappa input file match, or else we'd
// formally prove an error bound on the wrong calculation.
//
// Therefore, the Gappa input is generated in turn from a description of the
// calculation in Python, which can also be run in a mode that evaluates the
// calculation for specific inputs and prints the approximate quotient and many
// of the intermediate results leading up to it. This code itself also contains
// calls to the debugging macros in diagnostics.h, so that if you compile it
// with -DDIAGNOSTICS, it will print the same collection of intermediate
// values. So you can run both with matching inputs and check that all the
// values that should match, really do match.
//
// This gives confidence that the machine code matches the Python, the Python
// matches the Gappa, and therefore, Gappa's proven bound on the error applies
// to this machine code.

#include "endian.h"
#include "diagnostics.h"

  .syntax unified
  .text
  .p2align 2

  .globl arm_fp_ddiv
  .type arm_fp_ddiv,%function
arm_fp_ddiv:

  PUSH    {r4,r5,r6,r7,r8,lr}

  // Check if either input exponent 7FF (infinity or NaN), and if so, branch
  // out of line.
  LDR     r12, =0x07FF0000        // mask for exponent cold storage
  BICS    r4, r12, xh, LSR #4     // test for Infs or NaNs
  BICSNE  r4, r12, yh, LSR #4
  BEQ     ddiv_naninf

  // Extract the exponents of the input values x and y into bits 16..26 of r14
  // and r5 respectively, and in the process, check if either exponent is zero
  // (so that one or both inputs are 0 or denormal). In order to combine the
  // two tests, the second ANDS is performed conditionally, so that if x's
  // exponent is zero then the out-of-line code at ddiv_zerodenorm might find
  // y's exponent hasn't been set up yet.
  //
  // We also calculate the sign of the result, which will be needed whether or
  // not we branch. This is saved in the low bit of r4.
  ANDS    r4, r12, xh, LSR #4     // get exponent of x, setting Z if it's 0
  ANDSNE  r5, r12, yh, LSR #4     // if not, extract and test exponent of y
  EOR     r6, xh, yh              // XOR the input signs to get the result sign
  ORR     r4, r4, r6, LSR #31     // save it in the low bit of r4
  BEQ     ddiv_zerodenorm         // branch out of line for zeroes or denormals

  // Calculate the initial exponent of the result, by subtracting the two input
  // exponents and adjusting for the IEEE exponent bias. This value may have to
  // be adjusted by 1 later, depending on the quotient of the mantissas.
  //
  // If we branched to ddiv_zerodenorm above, and it found denormals but no
  // zeroes, it may branch back here after renormalising them. We expect the
  // out-of-line code to have left the exponent difference in the top half of
  // r4 (still with the output sign in the low bit), but not yet to have
  // applied the bias. So it branches back in immediately after the SUB.
  //
  // The exponent bias we want is either 0x3fe or 0x3ff, depending on whether
  // we have to shift the output mantissa by 1 below. Neither of those values
  // fits in the immediate field of an ADD instruction, so we must use two
  // instructions.
  SUB     r4, r4, r5
ddiv_normalised: // denormal handler will come back to here
  ADD     r4, r4, #0x03FC0000     // add the 8 high bits of the bias 0x3FE
  ADD     r4, r4, #0x00020000     // add the remaining bit of the bias

  // Shift both mantissas up to the top of their 64-bit register pair, and OR
  // in the leading 1 bit, which will occupy the high bit of the high word in
  // each case.
  MOV     r5, #1<<31              // high bit for ORing in to both mantissas
  ORR     xh, r5, xh, LSL #11     // shift up xh and OR in the high bit
  ORR     yh, r5, yh, LSL #11     // same for yh
  ORR     xh, xh, xl, LSR #21     // OR in the bits shifted out of xl into xh
  ORR     yh, yh, yl, LSR #21     // same for yl and yh
  LSL     xl, xl, #11             // shift up the rest of xl
  LSL     yl, yl, #11             // same for yl

  // Check if the two mantissas are exactly equal, so that the quotient is
  // exactly a power of 2. If so, branch out of line to handle that case
  // specially.
  //
  // This guarantees that when we examine the approximate quotient afterwards,
  // we can't be confused about whether it needs to be renormalised, which
  // would otherwise cost just as much effort as this check. Our reciprocal
  // approximation is always an underestimate (that's in the nature of this
  // particular Newton-Raphson iteration), so if x < y (meaning the mantissas
  // rather than the whole floats) then even the true quotient will be less
  // than 1, and the approximation even more so. On the other hand, if x > y,
  // then the true quotient will be enough greater than 1 that even the largest
  // possible error in the approximation can't make it look like less than 1.
  //
  // (Proof: regard x,y as normalised to the range [1,2). If x > y, then we
  // have x ≥ y+ε, where ε is the machine epsilon. So x/y ≥ 1+ε/y > 1+ε/2. And
  // the bound on the approximation error, given below, is far less than ε/2.)
  CMP     xh, yh
  CMPEQ   xl, yl
  BEQ     ddiv_result_is_power_of_2

  // Now we begin the actual calculation of the reciprocal approximation, so we
  // start issuing debug statements to print out the inputs, intermediate
  // results, and outputs.
  //
  // We begin with our two input mantissas stored in xh:xl and yh:yl, each with
  // its leading 1 explicit and shifted up to the top of the word. So they can
  // be regarded as 64-bit integers with the high bit set and the bottom 11
  // bits clear.
  //
  // The first diagnostics print the two mantissas in that integer form. This
  // is the form you'd also use to feed them to 'ddiv-prove.py --eval' to
  // confirm that the Python implementation of this algorithm does the same
  // thing.
  debug64 "x = 0x%016llX\n", xh, xl
  debug64 "y = 0x%016llX\n", yh, yl

  // Obtain an 8-bit reciprocal approximation by using the topmost 8 bits of y
  // as a lookup table. The top bit of y is always set, so there are only 128
  // lookup table entries, not 256. The 8-bit value we load also has its top
  // bit set.
  LSR     r5, yh, #24           // r5 is the table index plus 0x80
  ADR     r6, reciptbl-128      // so subtract 0x80 from the table address
  LDRB    r6, [r6, r5]          // and load the approximation into r6
  debug32 "recip08 = %02X\n", r6

  // First Newton-Raphson iteration, which expands that 8-bit approximation to
  // a 17-bit one, again with its top bit set. We use the top 16 bits of y for
  // this, so that we can fit the multiplications into ordinary MUL rather than
  // UMULL.
  //
  // The Newton-Raphson formula to turn an approximation r ≈ 1/y into a better
  // one is r → r(2-yr). In this case we're scaling up to integers (informal
  // fixed point), so the 2 becomes 2^24.
  LSR     r5, yh, #16           // get top halfword of y
  MUL     r7, r6, r5            // multiply it by the input value r
  RSB     r7, r7, #1<<24        // subtract from 2 (scaled up appropriately)
  MUL     r7, r6, r7            // multiply again to make r(2-yr)
  LSR     r7, r7, #14           // shift down to keep only 17 bits of it
  debug32 "recip16 = %04X\n", r7

  // Second iteration, expanding into a 32-bit reciprocal, using the top 31
  // bits of y (i.e. yh shifted by 1). The first multiplication (making yr) is
  // 32x32 → 64 bits, so we use a single UMULL; the second one making r(2-yr)
  // is 32x64, which we do with a UMULL by the bottom half of yr and then MLA
  // by the top half, so we only keep the low 64 bits of the full answer.
  //
  // The subtraction from 2 (again scaled up, this time to 2^48) is done by
  // RSBS+RSC, interleaved with the multiplications so as to use a delay slot
  // on CPUs that have one.
  LSR     r12, yh, #1
  UMULL   r6, r8, r7, r12       // r8:r6 = yr
  RSBS    r6, r6, #0            // low half of subtraction from 2
  UMULL   r12, lr, r7, r6       // multiply r by the low half of 2-yr
#if !__thumb__
  RSC     r8, r8, #1<<16        // high half of subtraction from 2
#else
  // Thumb has no RSC, so simulate it by bitwise inversion and then ADC
  MVN     r8, r8
  ADC     r8, r8, #1<<16
#endif
  MLA     r6, r7, r8, lr        // multiply r by the high half of 2-yr
  debug32 "recip32 = %08X\n", r6

  // Third iteration, expanding into a 64-bit reciprocal, with the leading bit
  // expected to end up in bit 60. Now the first multiplication to make yr is
  // 32x64 → 96 bits, so we put the product in three registers lr:r12:r8.
  // However, we're going to discard the low word r8 completely, because it
  // makes negligible difference. So we'll treat the output yr as 64-bit.
  UMULL   r8, r12, r6, yl       // multiply r by bottom half of y
  MOV     lr, #0                // initialize high word to 0
  UMLAL   r12, lr, r6, yh       // multiply r by top half of y
  // Subtract from a power of 2, as usual. But in this case the power of 2
  // we're subtracting from is 2^64, which is just off the top of the 64-bit
  // value in lr:r12. So in fact we're just negating the whole thing!
  //
  // To preserve the invariant that the approximation error is always negative,
  // we negate via one's complement rather than two's. (This would only make a
  // difference if r8 had happened to be exactly 0. That in turn can occur when
  // yl=0, so one of the test cases in ddiv-diagnostics.c deliberately uses
  // such a value, so that the intermediate results can be checked against the
  // reference Python.)
  MVN     r12, r12
  MVN     lr, lr
  // Now lr:r12:r8 contains 2-yr. We discard the low word r8 to reduce that to
  // 64 bits, and do another 32x64 → 96 bit multiplication.
  UMULL   r5, r8, r6, r12      // multiply r by bottom half of 2-yr
  MOV     r7, #0               // initialize high word to 0
  UMLAL   r8, r7, r6, lr       // multiply r by top half of 2-yr
  debug64 "recip64 = %016llX\n", r7, r8

  // That's the Newton-Raphson iteration done: we have a 64-bit approximation
  // to 1/y. Multiply it by x to get the full approximate quotient.
  //
  // In principle, this would be a 64x64 → 128 bit multiplication, involving
  // four long multiply instructions. But we only need the top 64 bits, and
  // we're already prepared to tolerate some error in the calculations, so we
  // cut corners: don't multiply the two low words together at all, and we
  // discard the bottom half of each of the (low * high) partial products
  // without bothering to propagate carries out of it.
  //
  // (All of these shortcuts are faithfully mimicked in the Python reference
  // implementation which generates Gappa input, so they're all accounted for
  // in the error analysis.)
#if __ARM_FEATURE_DSP
  UMULL   r12, r6, xh, r8      // r6 = high word of x * low word of 1/y
  UMULL   r12, r5, xl, r7      // r5 = low word of x * high word of 1/y
  UMAAL   r6, r5, xh, r7       // add those to the product of both high words
#else
  // Alternative instruction sequence using UMLAL, if UMAAL isn't available
  UMULL   r12, r6, xh, r8      // r6 = high word of x * low word of 1/y
  UMULL   r12, lr, xl, r7      // lr = low word of x * high word of 1/y
  ADDS    r6, r6, lr           // add those together
  MOV     r5, #0               // set r5 to the carry out of that addition
  ADC     r5, r5, #0
  UMLAL   r6, r5, xh, r7       // add that to the product of both high words
#endif
  // Now r5:r6 is the completed approximate quotient, with its leading bit at
  // position either 61 or 62.
  debug64 "quot = %016llX\n", r5, r6

  // Normalize so that the leading bit is always in bit 60, by shifting left if
  // it isn't there already, and adjusting the output exponent by 1 to
  // compensate.
  //
  // We do the test in a slightly tricky way, by arranging to set the V flag if
  // the leading bit is in bit 60. This allows us to do the left shift under
  // the VC condition, which is convenient because the LSLS instruction that
  // shifts the low word left moves the top bit into the C flag without
  // affecting V.
  //
  // We also save the value written into lr by the initial ADDS instruction,
  // because that contains enough information to tell us whether we
  // renormalised here. The correction path for quotients too close to a
  // rounding boundary will need to recover that information.
  ADDS    lr, r5, #0x40000000  // set V flag if bit 62 of high word set
  SUBVC   r4, r4, #1<<16       // if not, correct the exponent by 1,
  LSLSVC  r6, r6, #1           // shift the low word of the quotient left
  ADCVC   r5, r5, r5           // and shift its top bit into the high word

  // Now r5:r6 is the _normalised_ approximate quotient, with its leading bit
  // reliably in bit 60. This is the final output of the calculation that the
  // Gappa error-analysis proof applies to.
  debug64 "nquot = %016llX\n", r5, r6

  // That 64-bit output has bit 63 clear; the leading 1 bit of the output
  // mantissa in bit 62, followed by 52 more mantissa bits; then 10 bits at the
  // bottom which are used for determining rounding.
  //
  // Compute the _approximately_ rounded-to-nearest output mantissa, by adding
  // half a ULP and shifting down. If we don't go to the slow path, this is the
  // correct output mantissa. (See fdiv.S for the proof that the round-to-even
  // tiebreaking case can't occur in floating-point division.)
  //
  // We keep the original version of r6, containing the ten rounding bits, so
  // that we can test it to see if we need the slow path.
  ADDS    r7, r6, #1<<9        // add half a ULP, copying low word into r7
  ADC     r5, r5, #0           // propagate carry into high word
  LSR     r7, r7, #10          // shift low word right
  ORR     r7, r7, r5, LSL #22  // combine with bits shifted out of high word
  LSR     r5, r5, #10          // shift high word right

  // Now test r6 to see whether this output mantissa can be relied on, or
  // whether the approximation landed too close to a rounding boundary.
  //
  // The maximum possible error in the approximation, taking into account the
  // initial error in each lookup table entry, the remaining mathematical error
  // introduced by stopping after this many Newton-Raphson iterations, and
  // every shortcut, right shift, truncation and discarding of a partial
  // product in the algorithm above, is always negative, and less than 64 units
  // in the last place of the 64-bit approximate quotient. That is, the true
  // quotient lies somewhere between the 64-bit integer 'nquot' as output by
  // the debug statement above, and the larger value (nquot + 64), but it can't
  // be exactly equal to the latter.
  //
  // This can be confirmed by running the accompanying Python script, which
  // constructs input files to feed to Gappa, and then runs Rocq to check
  // Gappa's formal proof output:
  //
  // fp/aux/ddiv-prove.py --prove-error=-63.9
  //
  // (Passing a non-integer value for the error bound means we don't have to
  // worry about whether the end of the error interval is open or closed.)
  //
  // So if the bottom 10 bits of r6 have the value 2^9 or greater, we're safe,
  // because the true value is _larger_ than the approximation, so if the
  // approximation is already above the rounding boundary then so is the true
  // value. And if those 10 bits are (2^9-64) or less then we're also safe,
  // because even if the true value is greater by 63, it's still on the same
  // side of the rounding boundary.
  //
  // We check the error by subtracting (2^9-64), so that the dangerous values
  // of the bottom 10 bits are those in the range 0,...,63, i.e. precisely
  // those with none of bits 6,7,8,9 set.
  //
  // We also combine this test with a check for underflow, because that also
  // needs more careful handling (the mantissa must be re-rounded to a
  // different bit position, which involves knowing whether it's exact).
  // Underflow has happened if the exponent in the top half of r4 is negative
  // (it's off by 1 so that the leading mantissa bit will increment it), so we
  // test by an ASR#31 (copying the top bit of r4 into all of it) and negating.
  // That way, the output value is zero on underflow, matching the flags from
  // the other check.
  SUB     r6, r6, #(1<<9)-64
  TST     r6, #0x3C0              // now EQ means we must go to the slow path
  MVNSNE  r12, r4, ASR #31        // also set EQ if underflow has happened
  BEQ     ddiv_correction         // branch out of line to do the hard bit

  // If we do go to ddiv_correction, it branches back here after the correction
  // code has finished. Either way, we expect that r5:r7 is the result
  // mantissa, with the top bit set, already in the correct position in the
  // word, and already rounded to nearest.
ddiv_corrected:
  // Recombine the output mantissa with the sign and exponent.
  ADD     xh, r5, r4, LSL #31     // add sign bit to top word of mantissa
  BIC     r12, r4, #1             // isolate exponent in top half of r4
  ADD     xh, xh, r12, LSL #4     // add exponent to make the final high word
  MOV     xl, r7                  // move low word into the right register

  // If there's no overflow or underflow, we're done.
  //
  // We _identified_ underflow above when we went to the slow path, but having
  // done that, the slow path came back here, so we must check for it again.
  // (The only purpose of the detour was to obtain accurate information about
  // whether the quotient is exact, or needed rounding.)
  //
  // The output exponent, offset downwards by 1, is in the top half of r4. If
  // it's negative, there's an underflow; if it's too large, there's an
  // overflow. We do an approximate test for both at once via an unsigned
  // comparison against 0x7f0, using r12 (the register in which we already
  // cleared the sign bit stored at the bottom). This identifies _most_ normal
  // outputs as quickly as possible.
  //
  // 0x7f0 isn't the maximum possible known-safe exponent, but it's the largest
  // one that fits in the immediate field of CMP. We deal with the remaining
  // cases in the next few instructions.
  CMP     r12, #0x7f0 << 16
  POPLS   {r4,r5,r6,r7,r8,pc}

  // Now check the remaining cases more carefully.
  //
  // If r12 < 0 then we definitely have underflow. We detect overflow precisely
  // by seeing if the _final_ output exponent (in the output register xh) is
  // 0x7ff or more, by incrementing it and seeing if the sign is opposite from
  // the intended output sign.
  ADD     lr, xh, #1<<20          // increment the output exponent field
  TEQ     lr, r4, LSL #31         // set N if the sign now doesn't match r4[0]
  TSTPL   r12, r12                // otherwise, set N if underflow
  POPPL   {r4,r5,r6,r7,r8,pc}     // if neither, we've finished

  // If we still haven't returned, we really do have overflow or underflow, and
  // the sign of r12 tells us which.
  TST     r12, r12
  BMI     ddiv_underflow
  // For overflow, correct the sign by biasing the exponent downward, and go to
  // code that constructs an infinite return value (shared with the
  // division-by-zero handler).
  SUB     xh, xh, #0x60000000
  POP     {r4,r5,r6,r7,r8,lr}     // ddiv_retinf expects no regs on the stack
  B       ddiv_retinf

ddiv_correction:
  // The slow path, entered if the approximate quotient was too close to a
  // rounding boundary to trust, and also if there's a chance of underflow (so
  // that we can reliably determine the rounding direction, including whether
  // the quotient was exact).
  //
  // Regarding the input mantissas x,y and our approximate quotient q as
  // integers in [2^52,2^53), the quotient is an approximation to either
  // x*2^52/y or x*2^53/y, depending on which of x,y was larger. We know that q
  // is less than the true value of that quotient by at most a small fraction
  // of a ULP. So the correct rounded quotient is either equal to q or to q+1,
  // and we can decide which by multiplying back up by y: we want q - x*2^k/y
  // to be in the range (-1/2,+1/2) (where k = 52 or 53), which is equivalent
  // to asking if qy - x*2^k is in the range (-y/2,+y/2).
  //
  // That's a calculation we can do in integers using only addition and
  // multiplication. And we know that if q itself doesn't have that property
  // then q+1 will.

  // The mantissa of y is currently right at the top of the word, which means
  // that if the result of our check is greater than it, it will overflow. So
  // we must start by shifting y downward. We'll put it back at the bottom of
  // the word, where it was in the input float.
  LSR     yl, yl, #11             // shift yl right
  ORR     yl, yl, yh, LSL #21     // OR in the bits shifted out of yh
  LSR     yh, yh, #11             // shift yh right

  // Compute the integer qy-x. Because q is already very close to the right
  // quotient, we expect this to be an integer at most twice the size of y,
  // which easily fits in 64 bits. So we don't need to compute the full 128-bit
  // product: the low 64 bits are enough.
  UMULL   r8, r6, r7, yl          // 64-bit product of the low words
  MLA     r6, r7, yh, r6          //   + (high word of y) * (low word of q)
  MLA     r6, r5, yl, r6          //   + (high word of q) * (low word of y)

  // Now we must subtract either x << 53 or x << 52. This will only affect the
  // high word of the product we've just computed. Also the mantissa of x is
  // already shifted left by 11. So we shift xl left by either (52-32-11) or
  // (53-32-11), i.e. by 9 or by 10, and subtract from the high word of the
  // product.
  //
  // To decide which, we consult the value left in lr by the original test for
  // renormalization, which added 0x40000000 to the high word of the initial
  // approximate quotient 'quot'. If that had bit 62 set (so no renormalization
  // needed) then the addition carried into the sign bit; otherwise it didn't.
  // So lr is positive if and only if we need to shift xl left by an extra bit.
  TST     lr, lr                  // did we renormalize?
  SUBPL   r6, r6, xl, LSL #10     // if so, subtract x<<53 from q*y
  SUBMI   r6, r6, xl, LSL #9      // if not, subtract x<<52

  // Now r6:r8 contains the residual value r = qy - x*2^k as described above.
  // If this is between -y/2 and +y/2 then q is already the correctly rounded
  // quotient. Otherwise, the correct quotient is q+1, so the value in r6:r8
  // will be too small (incrementing q would add y to it). So we need to check
  // whether r < -y/2, or equivalently whether 2r < -y (avoiding having to
  // worry about what happens when we halve y if it's odd).
  //
  // As mentioned above, division can't give an exact halfway case, so we don't
  // need to worry about the case r = y/2.
  ADDS    r8, r8, r8              // multiply the residual by 2
  ADC     r6, r6, r6
  ADDS    lr, r8, yl              // add y to it, discarding the result
  ADCS    lr, r6, yh
  BPL     ddiv_corrected          // if the answer is positive, we're OK

  // If we didn't take that branch, then the approximate quotient is too small
  // by 1, so we must increment it. But also, we adjust the residual in r6:r8
  // to match. That residual is unused by the main epilogue code, but we also
  // came here for any underflowing value, and the underflow handler will need
  // the exact residual to determine the rounding direction.
  //
  // (We could re-test whether underflow had happened and use that to skip the
  // update of r6:r8, but the test would cost as much effort as it saved!)
  ADDS    r7, r7, #1              // increment the output quotient
  ADCS    r5, r5, #0
  ADDS    r8, r8, yl              // repeat the addition of y to the residual,
  ADCS    r6, r6, yh              //   this time keeping the result in r6:r8
  B       ddiv_corrected          // finally we can rejoin the main code

ddiv_result_is_power_of_2:
  // The special-case handler for the two input mantissas being equal, so that
  // the result is an exact power of two. We set up all the output registers to
  // the way the main code would have done it, and jump straight to
  // ddiv_corrected. This includes setting r6:r8 to the 'residual' value
  // computed by the slow path, in case this power-of-2 output is also an
  // underflow, which will depend on those registers.
  MOV     r5, #0x00100000         // high word of quotient mantissa = 1<<20
  MOV     r7, #0                  // low word of quotient mantissa = 0
  MOV     r6, #0                  // high word of residual = 0
  MOV     r8, #0                  // low word of residual = 0
  B       ddiv_corrected

ddiv_underflow:
  // We come here to handle underflow. The output double, constructed naïvely
  // from the out-of-range exponent, is in xh:xl. We expect in this situation
  // that we've _always_ come via either the ddiv_correction slow path or the
  // ddiv_result_is_power_of_2 special case, both of which will have set up a
  // residual value in r6:r8 equal to q*y - x*2^k (for appropriate k). This
  // value is positive if the quotient is slightly above the true value (i.e.
  // was rounded up), or negative if the quotient was rounded down. But we must
  // also distinguish the third case of the residual being exactly zero.
  ADD     xh, xh, #0x60000000     // apply IEEE 754 exponent bias for __dunder
  ORRS    r12, r6, r8             // set r12=0 and Z=1 if quotient was exact
  MOVNE   r12, #1                 // otherwise, set r12 = +1
  ORRNE   r12, r12, r6, ASR #31   // and change to -1 if residual is negative
  POP     {r4,r5,r6,r7,r8,lr}     // pop all locally saved registers
  B       __dunder                // and tailcall __dunder to finish

ddiv_zerodenorm:
  // We come here if either input had exponent 0, so there's at least one zero
  // or denormal. However, we know there are no infinities or NaNs, because
  // those were checked first and will have gone to ddiv_naninf below.
  //
  // First we must repeat the instruction which extracted the exponent of y
  // into r5, this time unconditionally, in case the setup code didn't do it.
  AND     r5, r12, yh, LSR #4

  // If either or both input is actually zero, the answer is easy.
  ORRS    lr, xl, xh, LSL #1    // is x zero?
  BEQ     ddiv_xzero
  ORRS    lr, yl, yh, LSL #1    // is y zero?
  BEQ     ddiv_divbyzero

  // Otherwise, delegate to __dnorm2 to handle denormals, converting them into
  // a normalised mantissa and an out-of-range exponent. __dnorm2 expects the
  // exponents at the bottom of their words instead of half way up, so shift
  // down first.
  LSR     r4, r4, #16
  LSR     r5, r5, #16
  PUSH    {r0, r1, r2, r3, r4, r5} // create a 'struct dnorm2' on the stack
  MOV     r0, sp                   // pass it by address
  BL      __dnorm2
  POP     {r0, r1, r2, r3, r4, r5}

  // Rejoin the main code, with the exponent difference in the top half of r4,
  // and the output sign in the low bit of r4. (The original setup code did the
  // latter, but we clobbered it while setting up for __dnorm2.)
  SUBS    r4, r4, r5               // exponent difference, at the bottom of r4
  LSLS    r4, r4, #16              // move it up to the right place
  ORR     r4, r4, r6, LSR #31      // recover output sign from top bit of r6
  B       ddiv_normalised          // rejoin the main code

ddiv_xzero:
  // We come here if x=0. We return 0 (of the right sign) if y is not 0, and
  // the default quiet NaN if both inputs are zero.
  ORRS    lr, yl, yh, LSL #1       // is y zero?
  BEQ     ddiv_ivo_pop             // if so, pop registers and return a NaN
  // We know xl=0 already, so we only need to reset xh to contain the right
  // output sign. The setup code left that in the high bit of r6.
  AND     xh, r6, #0x80000000
  POP     {r4,r5,r6,r7,r8,pc}

ddiv_divbyzero:
  // We come here if y=0, but x is not 0 (or we'd have gone to ddiv_xzero above
  // instead). So we're dividing a nonzero number by zero, and must return
  // infinity.
  POP     {r4,r5,r6,r7,r8,lr}
  EOR     xh, xh, yh               // combine signs to get result sign
  B       ddiv_retinf

ddiv_naninf:
  // We come here knowing that at least one operand is either NaN or infinity.
  // If there's a NaN, we can tailcall __dnan2 to do the right thing. Pop our
  // stacked registers first: we won't need that much spare space any more, and
  // it makes the tailcall easier if we've already done it.
  POP     {r4,r5,r6,r7,r8,lr}

  // A number is a NaN if its exponent is 0x7ff and at least one bit below that
  // is set. The CMP + ADC pair here converts the two words xh:xl into a single
  // word containing xh shifted up by one (throwing away the sign bit which
  // makes no difference), with its low bit set if xl was nonzero. So if that
  // is strictly greater than 0xffe00000, then x was a NaN.
  CMP     xl, #1
  ADC     r12, xh, xh
  CMP     r12, #0xFFE00000
  BHI     __dnan2
  // Now check y in the same way.
  CMP     yl, #1
  ADC     r12, yh, yh
  CMP     r12, #0xFFE00000
  BHI     __dnan2

  // Now we know there are no NaNs. Therefore there's at least one infinity. If
  // both operands are infinity then we have inf / inf = invalid operation and
  // must return a NaN. We detect this by XORing the inputs' exponent fields:
  // knowing one of them is 7FF, they XOR to zero iff the other one is too.
  EORS    r12, xh, yh              // XOR entire top words of the inputs
  LSL     r12, r12, #1             // shift left to discard the sign bit
  LSRS    r12, r12, #21            // shift right again to discard mantissas
  BEQ     ddiv_ivo                 // if what's left is 0, we have inf / inf

  // Otherwise, there's exactly one infinity, so our answers are easy, but
  // depend on which operand it is:
  //   infinity / anything = infinity
  //   anything / infinity = 0
  //
  // Determine if x is the infinity, by bitwise inverting the whole word and
  // then shifting left and right to isolate its exponent bits.
  MVN     r12, xh, LSL #1          // invert x, shift left to discard sign
  LSRS    r12, r12, #21            //   and shift right to discard mantissa
  EOR     xh, xh, yh               // calculate the output sign bit
  BEQ     ddiv_retinf              // if x = inf, return infinity of that sign
  MOV     xl, #0                   // otherwise clear all bits of x
  AND     xh, xh, #0x80000000      //   other than the sign bit
  BX      lr                       //   and return zero of the same sign
ddiv_retinf:
  // Construct and return an infinity in xh:xl, with whatever sign bit is
  // already in the top bit of xh.
  MOV     xl, #0                   // clear low word
  MVN     xh, xh, LSR #31          // shift xh[31] down to bit 0, inverted
  MVN     xh, xh, LSL #11          // uninvert, and put exponent 0x7ff below it
  LSL     xh, xh, #20              // shift back up to the top
  BX      lr

  // Code to construct and return the default quiet NaN, for the cases inf/inf
  // and 0/0. We provide two entry labels, one for callers who still need to
  // pop all the registers this function pushed, and one for callers who have
  // done that already.
ddiv_ivo_pop:
  POP     {r4,r5,r6,r7,r8,lr}
ddiv_ivo:
  MOVW    xh, 0x7ff8
  LSLS    xh, xh, #16
  MOV     xl, #0
  BX      lr

  // Table of approximate reciprocals, generated by running
  //
  // fp/aux/ddiv-prove.py --print-table
  //
  // which calculates the table entries using the same code that provides them
  // to the Gappa error analysis proof.
reciptbl:
  .byte 0xFF,0xFD,0xFB,0xF9,0xF7,0xF5,0xF4,0xF2
  .byte 0xF0,0xEE,0xED,0xEB,0xE9,0xE8,0xE6,0xE4
  .byte 0xE3,0xE1,0xE0,0xDE,0xDD,0xDB,0xDA,0xD8
  .byte 0xD7,0xD5,0xD4,0xD3,0xD1,0xD0,0xCF,0xCD
  .byte 0xCC,0xCB,0xCA,0xC8,0xC7,0xC6,0xC5,0xC4
  .byte 0xC2,0xC1,0xC0,0xBF,0xBE,0xBD,0xBC,0xBB
  .byte 0xBA,0xB9,0xB8,0xB7,0xB6,0xB5,0xB4,0xB3
  .byte 0xB2,0xB1,0xB0,0xAF,0xAE,0xAD,0xAC,0xAB
  .byte 0xAA,0xA9,0xA8,0xA8,0xA7,0xA6,0xA5,0xA4
  .byte 0xA3,0xA3,0xA2,0xA1,0xA0,0x9F,0x9F,0x9E
  .byte 0x9D,0x9C,0x9C,0x9B,0x9A,0x99,0x99,0x98
  .byte 0x97,0x97,0x96,0x95,0x95,0x94,0x93,0x93
  .byte 0x92,0x91,0x91,0x90,0x8F,0x8F,0x8E,0x8E
  .byte 0x8D,0x8C,0x8C,0x8B,0x8B,0x8A,0x89,0x89
  .byte 0x88,0x88,0x87,0x87,0x86,0x85,0x85,0x84
  .byte 0x84,0x83,0x83,0x82,0x82,0x81,0x81,0x80

  .size arm_fp_ddiv, .-arm_fp_ddiv
