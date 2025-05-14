/*
 * Tests of IEEE 754 single-precision to int64 conversion (round towards zero)
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
    uint32_t in;
    uint64_t out;
};

static const struct test tests[] = {
  // Tests that don't depend on Arm-specific handling of invalid operations
  { 0x00000000, 0x0000000000000000 },
  { 0x00000001, 0x0000000000000000 },
  { 0x00000001, 0x0000000000000000 },
  { 0x00500000, 0x0000000000000000 },
  { 0x00500000, 0x0000000000000000 },
  { 0x3e800000, 0x0000000000000000 },
  { 0x3f000000, 0x0000000000000000 },
  { 0x3f400000, 0x0000000000000000 },
  { 0x3f800000, 0x0000000000000001 },
  { 0x3fa00000, 0x0000000000000001 },
  { 0x3fc00000, 0x0000000000000001 },
  { 0x3fe00000, 0x0000000000000001 },
  { 0x40000000, 0x0000000000000002 },
  { 0x40100000, 0x0000000000000002 },
  { 0x40200000, 0x0000000000000002 },
  { 0x40300000, 0x0000000000000002 },
  { 0x55023450, 0x0000082345000000 },
  { 0x5effffff, 0x7fffff8000000000 },
  { 0x80000000, 0x0000000000000000 },
  { 0x80000001, 0x0000000000000000 },
  { 0x80000001, 0x0000000000000000 },
  { 0x80500000, 0x0000000000000000 },
  { 0x80500000, 0x0000000000000000 },
  { 0xbe800000, 0x0000000000000000 },
  { 0xbf000000, 0x0000000000000000 },
  { 0xbf400000, 0x0000000000000000 },
  { 0xbf800000, 0xffffffffffffffff },
  { 0xbfa00000, 0xffffffffffffffff },
  { 0xbfc00000, 0xffffffffffffffff },
  { 0xbfe00000, 0xffffffffffffffff },
  { 0xc0000000, 0xfffffffffffffffe },
  { 0xc0100000, 0xfffffffffffffffe },
  { 0xc0200000, 0xfffffffffffffffe },
  { 0xc0300000, 0xfffffffffffffffe },
  { 0xdf000000, 0x8000000000000000 },

  // Tests that do depend on Arm-specific choices
  { 0x5f000000, 0x7fffffffffffffff },
  { 0x7f800000, 0x7fffffffffffffff },
  { 0x7fa111d3, 0x0000000000000000 },
  { 0x7febfdda, 0x0000000000000000 },
  { 0xdf000001, 0x8000000000000000 },
  { 0xff800000, 0x8000000000000000 },
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
      uint64_t outbits = (uint64_t)(int64_t)in;
#else
      extern uint64_t arm_fp_f2lz(uint32_t);
      uint64_t outbits = arm_fp_f2lz(t->in);
#endif

      if (outbits != t->out)
	{
	  printf ("FAIL: f2lz(%08" PRIx32 ") -> %016" PRIx64
		  ", expected %016" PRIx64 "\n", t->in, outbits, t->out);
	  failed = true;
	}
    }

  if (!failed)
    printf ("all passed\n");

  return failed;
}
