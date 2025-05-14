/*
 * Tests of IEEE 754 single-precision to uint32 conversion (round towards zero)
 *
 * Copyright (c) 1999-2025, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

struct test
{
  uint32_t in, out;
};

static const struct test tests[] = {
  // Tests that don't depend on Arm-specific handling of invalid operations
  { 0x00000000, 0x00000000 },
  { 0x00000001, 0x00000000 },
  { 0x00000001, 0x00000000 },
  { 0x00500000, 0x00000000 },
  { 0x00500000, 0x00000000 },
  { 0x3e800000, 0x00000000 },
  { 0x3f000000, 0x00000000 },
  { 0x3f400000, 0x00000000 },
  { 0x3f800000, 0x00000001 },
  { 0x3fa00000, 0x00000001 },
  { 0x3fc00000, 0x00000001 },
  { 0x3fe00000, 0x00000001 },
  { 0x40000000, 0x00000002 },
  { 0x40100000, 0x00000002 },
  { 0x40200000, 0x00000002 },
  { 0x40300000, 0x00000002 },
  { 0x4f7fffff, 0xffffff00 },
  { 0x80000000, 0x00000000 },
  { 0xbf7fffff, 0x00000000 },

  // Tests that do depend on Arm-specific choices
  { 0x4f800000, 0xffffffff },
  { 0x7f800000, 0xffffffff },
  { 0x7fa111d3, 0x00000000 },
  { 0x7febfdda, 0x00000000 },
  { 0xbf800000, 0x00000000 },
  { 0xc0000000, 0x00000000 },
  { 0xff800000, 0x00000000 },
};

float
make_float (uint32_t x)
{
  float r;
  memcpy (&r, &x, sizeof (r));
  return r;
}

uint32_t
unmake_float (float x)
{
  uint32_t r;
  memcpy (&r, &x, sizeof (r));
  return r;
}

int
main (void)
{
  bool failed = false;

  for (size_t i = 0; i < sizeof (tests) / sizeof (tests[0]); i++)
    {
      const struct test *t = &tests[i];

#ifdef USE_NATIVE_ARITHMETIC
      /* If you compile with USE_NATIVE_ARITHMETIC defined, the same
       * set of tests will be run using the toolchain's built in float
       * arithmetic, instead of calling arm_fp_f2*.
       *
       * I have to declare the input float as volatile, to inhibit the
       * compiler from potentially optimizing away the whole of main()
       * on the grounds that some of the tests overflow! */
      volatile float in = make_float (t->in);
      uint32_t outbits = in;
#else
      extern uint32_t arm_fp_f2uiz(uint32_t);
      uint32_t outbits = arm_fp_f2uiz(t->in);
#endif

      if (outbits != t->out)
	{
	  printf ("FAIL: f2uiz(%08" PRIx32 ") -> %08" PRIx32
		  ", expected %08" PRIx32 "\n", t->in, outbits, t->out);
	  failed = true;
	}
    }

  if (!failed)
    printf ("all passed\n");

  return failed;
}
