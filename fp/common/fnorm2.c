// Helper function for handling single-precision input denormals.
//
// Copyright (c) 2025, Arm Limited.
// SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception

// This helper function is available for use by single-precision float
// arithmetic implementations, to handle denormal inputs on entry by
// renormalizing the mantissa and modifying the exponent to match.

// Structure containing the function's inputs and outputs.
//
// On entry: a, b are two input floating-point numbers, still in IEEE 754
// encoding. expa and expb are the 8-bit exponents of those numbers, extracted
// and shifted down to the low 8 bits of the word, with no other change.
// Neither value should be zero, or have the maximum exponent (indicating an
// infinity or NaN).
//
// On exit: each of a and b contains the mantissa of the input value, with the
// leading 1 bit made explicit, and shifted up to the top of the word. If expa
// was zero (indicating that a was denormal) then it is now represented as a
// normalized number with an out-of-range exponent (zero or negative). The same
// applies to expb and b.
struct fnorm2
{
  unsigned a, b, expa, expb;
};

void
__fnorm2 (struct fnorm2 *values)
{
  values->a <<= 8;
  values->b <<= 8;
  if (values->expa == 0)
    {
      unsigned shift = __builtin_clz (values->a);
      values->a <<= shift;
      values->expa = 1 - shift;
    }
  else
    {
      values->a |= 0x80000000;
    }
  if (values->expb == 0)
    {
      unsigned shift = __builtin_clz (values->b);
      values->b <<= shift;
      values->expb = 1 - shift;
    }
  else
    {
      values->b |= 0x80000000;
    }
}
