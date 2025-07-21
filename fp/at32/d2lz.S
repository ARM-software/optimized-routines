// Double-precision float to int64 conversion, rounded towards zero.
//
// Copyright (c) 1994-1998,2025, Arm Limited.
// SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception

#include "endian.h"

  .syntax unified
  .text
  .p2align 2

  .globl arm_fp_d2lz
  .type arm_fp_d2lz,%function
arm_fp_d2lz:

  // The fast path: deliver an answer as quickly as possible for positive cases
  // that don't overflow, and branch out of line to handle everything else more
  // slowly, including negative numbers, overflows, and NaNs.
  //
  // The basic idea is to make a bare version of the mantissa, with its leading
  // 1 bit explicit at the top of the word, and shift it right by an amount
  // derived from the exponent.

  // Shift the exponent down to the bottom of the word. Using ASR here in place
  // of LSR means the sign bit keeps its original value, and therefore as a
  // side effect the N flag will tell us whether the input was negative. On the
  // positive path the output is the same anyway.
  ASRS    r2, ah, #20

  // Make the 64-bit mantissa word, with its top half in r3 and the bottom half
  // in r12, and set the leading mantissa bit at the top of r3.
  //
  // (In principle, the leading mantissa bit shouldn't be set at all if the
  // input exponent is zero, because then the input is either 0 or a denormal.
  // But since we're always rounding towards zero and not delivering any output
  // indicating whether the result is inexact, it makes no difference - all
  // denormals will round down to 0 anyway, and will still do so even if we
  // imagine that their leading mantissa bit was set.)
  LSL     r3, ah, #11           // shift top word left to discard sign+exponent
  ORR     r3, r3, al, LSR #21   // combine with bits from bottom word
  ORR     r3, r3, #1<<31        // set the leading bit
  LSL     r12, al, #11          // shift bottom word left

  // Branch out of line for negative cases, using the N flag set by the ASRS at
  // the start of the function.
  //
  // For 64-bit outputs it's difficult to handle both signs in a shared code
  // path. Converting to a 32-bit integer you can finish up with an RSBMI to
  // conditionally negate it, but negating a 64-bit number requires two
  // instructions and clobbers the flags in between, so you can't
  // conditionalize both instructions on the same prior state of the flags.
  BMI     d2lz_neg

  // Convert the mantissa into a right shift count, indicating how many bits we
  // want to shift the 64-bit mantissa in r3:r12 right by.
  //
  // If that's negative or zero, that means the input is too big, or the
  // exponent is 0x7FF (so we might have a NaN), so branch out of line again.
  //
  // Another possibility is that if the number is very small, the right shift
  // count might not fit in a byte, in which case the AArch32 shift semantics
  // might do the wrong thing, since they only look at the low byte.
  //
  // To spot this efficiently, we do the conversion in two steps, arranged so
  // that the first step detects massive underflow and the second detects
  // overflow. We needed two steps anyway, because we need to subtract the
  // input exponent from 0x43e (the exponent of input numbers between 2^63 and
  // 2^64), and that value doesn't fit in an immediate field. So the first step
  // subtracts 0x340, which makes r2 become negative if the exponent is very
  // small; then the second step subtracts from 0xfe, giving 0xfe-(exp-0x340))
  // = 0x43e-exp.
  //
  // The input is tiny if r2 is negative after the first subtraction. In that
  // situation we set r2 to 0 before the second step, treating all exponents
  // smaller than 0x340 as if they had been 0x340. We do this by ASR+BIC, which
  // avoids needing an IT instruction in Thumb.
  SUB     r2, r2, #0x340        // first stage: r2 is negative for underflow
  BIC     r2, r2, r2, ASR #31   // if so, saturate by clearing all bits of r2
  RSBS    r2, r2, #0xfe         // second step: r2 is now the right shift count
  BLE     d2lz_invalid          // if it was negative or zero, overflow

  // Make the top word of the result, which is the easy part: it's a
  // shifted-right version of the top word of the mantissa, which will be zero
  // if the shift count is 32 or more.
  LSRS    ah, r3, r2

  // Make the bottom word of the result. If the shift count is less than 32,
  // this will be (ml >> n) | (mh << (32-n)), where 'mh' and 'ml' denote the
  // high and low words of the mantissa (in r3 and r12 respectively). Otherwise
  // the whole high output word is zero, and the low mantissa word contributes
  // nothing to the output, so we just need mh >> (n-32).
  LSR     al, r12, r2           // start by setting it to ml >> n
  RSBS    r2, r2, #32           // compute 32-n, and find which case we're in
#if !__thumb__
  ORRHI   al, al, r3, LSL r2    // shift count < 32, so OR in ml << (32-n)
#else
  // In Thumb we must separate the register-controlled shift and the OR into
  // two instructions.
  LSLHI   r3, r3, r2
  ORRHI   al, al, r3
#endif
  RSBLS   r2, r2, #0            // shift count > 32, so compute n-32
  LSRLS   al, r3, r2            // replace al with mh >> (n-32)

  // We're done: the result is already rounded towards zero.
  BX      lr

d2lz_neg:
  // Handle negative numbers. We come here with the mantissa already prepared
  // in r3 and r12, and the exponent in the bottom 11 bits of r2 with all 1s
  // above it (because it was shifted down via ASR and the sign bit was set).
  // In other words, r2 = exponent - 0x800.

  // Convert the exponent into a shift count, exactly as on the positive path
  // except that the initial subtraction of 0x340 is adjusted to cancel out the
  // offset of 0x800 in r2, so that it becomes an addition.
  ADD     r2, r2, #0x800-0x340  // first stage: r2 is negative for underflow
  BIC     r2, r2, r2, ASR #31   // if so, saturate by clearing all bits of r2
  RSBS    r2, r2, #0xfe         // second step: r2 is now the right shift count
  BLE     d2lz_invalid          // if it was negative or zero, overflow

  // Shift the mantissa down to create the absolute value of the result in
  // ah:al, exactly as on the positive path above.
  LSRS    ah, r3, r2            // make high word of mantissa
  LSR     al, r12, r2           // initially set low word to ml >> n
  RSBS    r2, r2, #32           // compute 32-n, and find which case we're in
#if !__thumb__
  ORRHI   al, al, r3, LSL r2    // shift count < 32, so OR in ml << (32-n)
#else
  // In Thumb we must separate the register-controlled shift and the OR into
  // two instructions.
  LSLHI   r3, r3, r2
  ORRHI   al, al, r3
#endif
  RSBLS   r2, r2, #0            // shift count > 32, so compute n-32
  LSRLS   al, r3, r2            // and replace al with mh >> that

  // All of that was almost exactly the same as the positive code path, but now
  // we must negate the answer before returning it.
  RSBS    al, al, #0            // negate low word, setting carry flag
#if !__thumb__
  RSC     ah, ah, #0            // negate high word
#else
  // Thumb has no RSC, so substitute MVN + ADC.
  MVN     ah, ah
  ADC     ah, ah, #0
#endif

  // We're done.
  BX      lr

d2lz_invalid:
  // We come here if the exponent field of the number is large enough that it's
  // either a NaN or infinity, or a finite number of absolute value at least
  // 2^63.
  //
  // For out-of-range positive values, we return the maximum positive signed
  // integer 0x7fffffffffffffff. For out-of-range negative values, we return
  // the minimum negative signed integer 0x8000000000000000. For NaNs, we
  // return zero.
  //
  // Not _every_ number of this kind is actually an invalid input. The exact
  // value -2^63 is perfectly valid. If this implementation supported FP
  // exceptions, we'd have to detect that one case and return
  // 0x8000000000000000 with no exception, while raising an Invalid Operation
  // exception for everything else. But since we don't support exceptions, we
  // don't have to tell the difference here: -2^63 and negative overflows both
  // return 0x8000000000000000, and it doesn't matter that one is the right
  // answer and the other a best-effort error response.

  // Check for NaNs and branch out of line.
  CMP     al, #1                  // set C if any bit of al is nonzero
  ADC     r2, ah, ah              // shift that bit into ah, discarding sign
  CMN     r2, #1 << 21            // then greater than 0xFFE00000 means NaN
  BHI     d2lz_nan

  // The remaining case is a too-large exponent. Return either INT_MAX or
  // INT_MIN depending on sign, using the fact that (ah ASR 31) is 0 for a
  // positive input or 0xFFFFFFFF for a negative input, so it's exactly the
  // bitwise inverse of the correct low word of the output.
  MVN     al, ah, ASR #31         // make the low word of the output
  EOR     ah, al, #0x80000000     // make the high word by flipping sign bit
  BX      lr

d2lz_nan:
  // Return zero, for use when the input was a NaN.
  MOVS    ah, #0
  MOVS    al, #0
  BX      lr

  .size arm_fp_d2lz, .-arm_fp_d2lz
