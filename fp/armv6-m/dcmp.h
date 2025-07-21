// Shared code between most of the double-precision comparison functions.
//
// Copyright (c) 1994-1998,2025, Arm Limited.
// SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception

// --------------------------------------------------
// How to use this header file:
//
// The code below is the skeleton of a double-precision FP compare, with two
// details left out: which input value is in which register, and what (if
// anything) should be written to r0 as a return value.
//
// This header file is expected to be #included from inside a function
// definition in a .S file. The source file including this header should
// provide the following:
//
// op0h, op0l, op1h, op1l: register aliases (via .req) for the registers
// containing the input operands.
//  - For most comparisons, op0h,op0l will correspond to ah,al, and op1h,op1l
//    to bh,bl (as defined in turn in endian.h).
//  - But a function with the reversed semantics of __aeabi_cdrcmple wil define
//    them the other way round.
//
// SetReturnRegister: an assembly macro that looks at the PSR flags and sets up
// an appropriate return value in r0, for the cases that do *not* involve NaN.
//  - On entry to this macro, the condition codes LO, EQ and HI indicate that
//    op0 < op1, op0 == op1 or op0 > op1 respectively.
//  - For functions that return a result in the flags, this macro can be empty,
//    because those are the correct flags to return anyway.
//  - Functions that return a boolean in r0 should set it up by checking the
//    flags.
//
// dcmp_NaN: a label defined within the compare function, after the #include of
// this header. Called when at least one input is a NaN, and sets up the
// appropriate return value for that case.

// --------------------------------------------------
// The actual entry point of the compare function.
//
// The basic plan is to start by ORing together the two inputs. This tells us
// two things:
//  - the top bit of the output tells us whether both inputs are positive, or
//    whether at least one is negative
//  - if the 11 exponent bits of the output are not all 1, then there are
//    definitely no NaNs, so a fast path can handle most non-NaN cases.

  PUSH    {r4,r5,r6,lr}

  // Set up the constant 1 << 20 in a register, which we'll need on all
  // branches.
  MOVS    r5, #1
  LSLS    r5, r5, #20

  // First diverge control for the negative-numbers case.
  MOVS    r4, op0h
  ORRS    r4, r4, op1h
  BMI     dcmp_negative         // high bit set => at least one negative input

  // Here, both inputs are positive. Try adding 1<<20 to their bitwise OR in
  // r4. This will carry all the way into the top bit, setting the N flag, if
  // all 11 exponent bits were set.
  CMN     r4, r5
  BMI     dcmp_NaNInf_check_positive // need to look harder for NaNs

  // The fastest fast path: both inputs positive and we could easily tell there
  // were no NaNs. So we just compare op0 and op1 as unsigned integers.
  CMP     op0h, op1h
  BEQ     dcmp_low_word_positive
  SetReturnRegister
  POP     {r4,r5,r6,pc}
dcmp_low_word_positive:
  CMP     op0l, op1l
  SetReturnRegister
  POP     {r4,r5,r6,pc}

dcmp_NaNInf_check_positive:
  // Second tier for positive numbers. We come here if both inputs are
  // positive, but our fast initial check didn't manage to rule out a NaN. But
  // it's not guaranteed that there _is_ a NaN, for two reasons:
  //
  //  1. An input with exponent 0x7FF might be an infinity instead. Those
  //     behave normally under comparison.
  //
  //  2. There might not even _be_ an input with exponent 0x7FF. All we know so
  //     far is that the two inputs ORed together had all the exponent bits
  //     set. So each of those bits is set in _at least one_ of the inputs, but
  //     not necessarily all in the _same_ input.
  //
  // Test each exponent individually for 0x7FF, using the same CMN idiom as
  // above. If neither one carries into the sign bit then we have no NaNs _or_
  // infinities and can compare the registers and return again.
  CMN     op0h, r5
  BMI     dcmp_NaN_check_positive
  CMN     op1h, r5
  BMI     dcmp_NaN_check_positive

  // Second-tier return path, now we've ruled out anything difficult. By this
  // time we know that the two operands have different exponents (because the
  // exponents' bitwise OR is 0x7FF but neither one is 0x7FF by itself, so each
  // must have a set bit not present in the other). So we only need to compare
  // the high words.
  CMP     op0h, op1h
  SetReturnRegister
  POP     {r4,r5,r6,pc}

dcmp_NaN_check_positive:
  // Third tier for positive numbers. Here we know that at least one of the
  // inputs has exponent 0x7FF. But they might still be infinities rather than
  // NaNs. So now we must check whether there's an actual NaN.
  //
  // We do this by shifting the high word of each input left to get rid of the
  // sign bit, shifting a bit in at the bottom which is 1 if any bit is set in
  // the low word. Then we check if the result is _greater_ than 0xFFE00000
  // (but not equal), via adding 0x00200000 to it and testing for the HI
  // condition (carry flag set, but Z clear).
  //
  // We could have skipped the second-tier check and done this more rigorous
  // test immediately. But that would cost an extra instruction in the case
  // where there are no infinities or NaNs, and we assume that that is so much
  // more common that it's worth optimizing for.
  LSLS    r6, r5, #1         // set r6 = 1<<21
  CMP     op0l, #1           // set C if op0l is nonzero
  ADCS    op0h, op0h, op0h   // shift op0h left, bringing in the C bit
  CMN     op0h, r6           // if HI, then op0 is a NaN
  BHI     dcmp_NaN
  CMP     op1l, #1           // set C if op1l is nonzero
  ADCS    op1h, op1h, op1h   // shift op1h left, bringing in the C bit
  CMN     op1h, r6           // if HI, then op1 is a NaN
  BHI     dcmp_NaN

  // Now we've finally ruled out NaNs! And we still know both inputs are
  // positive. So the third-tier return path can just compare the top words
  // again. (The fact that we've just shifted them left doesn't make a
  // difference.)
  CMP     op0h, op1h
  SetReturnRegister
  POP     {r4,r5,r6,pc}

dcmp_negative:
  // We come here if at least one operand is negative. We haven't checked for
  // NaNs at all yet (the sign check came first), so repeat the first-tier
  // check strategy of seeing if all exponent bits are set in r12.
  //
  // On this path, the sign bit in r12 is set, so if adding 1 to the low
  // exponent bit carries all the way through into the sign bit, it will
  // _clear_ the sign bit rather than setting it. So we expect MI to be the
  // "definitely no NaNs" result, where it was PL on the positive branch.
  CMN     r4, r5
  BPL     dcmp_NaNInf_check_negative

  // Now we have no NaNs, but at least one negative number. This gives us two
  // complications:
  //
  //  1. Floating-point numbers are sign/magnitude, not two's complement, so we
  //     have to consider separately the cases of "both negative" and "one of
  //     each sign".
  //
  //  2. -0 and +0 are required to compare equal.
  //
  // But problem #1 is not as hard as it sounds! If both operands are negative,
  // then we can get the result we want by comparing them as unsigned integers
  // the opposite way round, because the input with the smaller value (as an
  // integer) is the larger number in an FP ordering sense. And if one operand
  // is negative and the other is positive, the _same_ reversed comparison
  // works, because the positive number (with zero sign bit) will always
  // compare less than the negative one in an unsigned-integers sense.
  //
  // So we only have to worry about problem #2, signed zeroes. This only
  // affects the answer if _both_ operands are zero. So we check that by
  // testing all bits of both operands apart from the sign bit.
  LSLS    r6, r4, #1         // logical OR of both high words except the signs
  ORRS    r6, r6, op0l       // combine that with the low word of op0
  ORRS    r6, r6, op1l       // and op1, so now only EQ if both are zero
  BEQ     dcmp_equal
  // Now we've ruled out confusing zero cases, just compare the operands in
  // reverse sense.
  CMP     op1h, op0h
  BEQ     dcmp_low_word_negative
  SetReturnRegister
  POP     {r4,r5,r6,pc}
dcmp_low_word_negative:
  CMP     op1l, op0l
  SetReturnRegister
  POP     {r4,r5,r6,pc}

dcmp_equal:
  // We come here if we know the inputs are supposed to compare equal. Set up
  // the flags by comparing a register with itself.
  //
  // (We might have come here via a BEQ, in which case we know Z=1, but we also
  // need C=1 for our caller to get _all_ the right flags.)
  CMP     r0, r0             // compare a register with itself
  SetReturnRegister
  POP     {r4,r5,r6,pc}

dcmp_NaNInf_check_negative:
  // Second tier for negative numbers: we know the OR of the exponents is 0xFF,
  // but again, we might not have either _actual_ exponent 0xFF, and also, an
  // exponent 0xFF might be an infinity instead of a NaN.
  //
  // On this path we've already branched twice (once for negative numbers and
  // once for the first-tier NaN check), so we'll just go straight to the
  // precise check for NaNs.
  //
  // Like the dcmp_NaNInf_check_positive case, we do each NaN check by making a
  // word consisting of (high word << 1) OR (1 if low word is nonzero). But
  // unlike the positive case, we can't make those words _in place_,
  // overwriting op0h and op1h themselves, because that would shift the sign
  // bits off the top, and we still need the sign bits to get the comparison
  // right. (In the positive case, we knew both sign bits were 0, enabling a
  // shortcut.)
  LSLS    r6, r5, #1         // set r6 = 1<<21
  MOVS    r4, op0h           // copy op0h into a scratch register to modify
  CMP     op0l, #1           // set C if op0l is nonzero
  ADCS    r4, r4, r4         // shift left, bringing in the C bit
  CMN     r4, r6             // if HI, then op0 is a NaN
  BHI     dcmp_NaN
  MOVS    r4, op1h           // copy op1h into a scratch register to modify
  CMP     op1l, #1           // set C if op1l is nonzero
  ADCS    r4, r4, r4         // shift left, bringing in the C bit
  CMN     r4, r6             // if HI, then op1 is a NaN
  BHI     dcmp_NaN

  // Now we've ruled out NaNs, so we can just compare the two input registers
  // and return. On this path we _don't_ need to check for the special case of
  // comparing two zeroes, because we only came here if the bitwise OR of the
  // exponent fields was 0x7FF, which means the exponents can't both have been
  // zero! So we can _just_ do the reversed CMP and finish.
  CMP     op1h, op0h
  SetReturnRegister
  POP     {r4,r5,r6,pc}
