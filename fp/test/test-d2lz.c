/*
 * Tests of IEEE 754 double-precision to int64 conversion (round towards zero)
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
  uint64_t in, out;
};

static const struct test tests[] = {
  // Tests that don't depend on Arm-specific handling of invalid operations
  { 0x0000000000000000, 0x0000000000000000 },
  { 0x0000000000000001, 0x0000000000000000 },
  { 0x0000000000500000, 0x0000000000000000 },
  { 0x3fd0000000000000, 0x0000000000000000 },
  { 0x3fe0000000000000, 0x0000000000000000 },
  { 0x3fe8000000000000, 0x0000000000000000 },
  { 0x3ff0000000000000, 0x0000000000000001 },
  { 0x3ff4000000000000, 0x0000000000000001 },
  { 0x3ff8000000000000, 0x0000000000000001 },
  { 0x3ffc000000000000, 0x0000000000000001 },
  { 0x4000000000000000, 0x0000000000000002 },
  { 0x4002000000000000, 0x0000000000000002 },
  { 0x4004000000000000, 0x0000000000000002 },
  { 0x4006000000000000, 0x0000000000000002 },
  { 0x41f0000000040000, 0x0000000100000000 },
  { 0x41f0000000080000, 0x0000000100000000 },
  { 0x41f00000000c0000, 0x0000000100000000 },
  { 0x41f0000000140000, 0x0000000100000001 },
  { 0x41f0000000180000, 0x0000000100000001 },
  { 0x41f00000001c0000, 0x0000000100000001 },
  { 0x41f0000000240000, 0x0000000100000002 },
  { 0x41f0000000280000, 0x0000000100000002 },
  { 0x41f00000002c0000, 0x0000000100000002 },
  { 0x41fffffffff40000, 0x00000001ffffffff },
  { 0x41fffffffff80000, 0x00000001ffffffff },
  { 0x41fffffffffc0000, 0x00000001ffffffff },
  { 0x42a0468ace000000, 0x0000082345670000 },
  { 0x43dfffffffffffff, 0x7ffffffffffffc00 },
  { 0x8000000000000000, 0x0000000000000000 },
  { 0x8000000000000001, 0x0000000000000000 },
  { 0x8000000000500000, 0x0000000000000000 },
  { 0xbfd0000000000000, 0x0000000000000000 },
  { 0xbfe0000000000000, 0x0000000000000000 },
  { 0xbfe8000000000000, 0x0000000000000000 },
  { 0xbff0000000000000, 0xffffffffffffffff },
  { 0xbff4000000000000, 0xffffffffffffffff },
  { 0xbff8000000000000, 0xffffffffffffffff },
  { 0xbffc000000000000, 0xffffffffffffffff },
  { 0xc000000000000000, 0xfffffffffffffffe },
  { 0xc002000000000000, 0xfffffffffffffffe },
  { 0xc004000000000000, 0xfffffffffffffffe },
  { 0xc006000000000000, 0xfffffffffffffffe },
  { 0xc1f0000000040000, 0xffffffff00000000 },
  { 0xc1f0000000080000, 0xffffffff00000000 },
  { 0xc1f00000000c0000, 0xffffffff00000000 },
  { 0xc1f0000000140000, 0xfffffffeffffffff },
  { 0xc1f0000000180000, 0xfffffffeffffffff },
  { 0xc1f00000001c0000, 0xfffffffeffffffff },
  { 0xc1f0000000240000, 0xfffffffefffffffe },
  { 0xc1f0000000280000, 0xfffffffefffffffe },
  { 0xc1f00000002c0000, 0xfffffffefffffffe },
  { 0xc1fffffffff40000, 0xfffffffe00000001 },
  { 0xc1fffffffff80000, 0xfffffffe00000001 },
  { 0xc1fffffffffc0000, 0xfffffffe00000001 },
  { 0xc3dfffffffffffff, 0x8000000000000400 },
  { 0xc3e0000000000000, 0x8000000000000000 },

  // Tests that do depend on Arm-specific choices
  { 0x43e0000000000000, 0x7fffffffffffffff },
  { 0x7ff0000000000000, 0x7fffffffffffffff },
  { 0x7ff6d1ebdfe15ee3, 0x0000000000000000 },
  { 0x7ff9a4da74944a09, 0x0000000000000000 },
  { 0xc3e0000000000001, 0x8000000000000000 },
  { 0xfff0000000000000, 0x8000000000000000 },
};

double
make_double (uint64_t x)
{
  double r;
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
       * arithmetic, instead of calling arm_fp_d2*.
       *
       * I have to declare the input float as volatile, to inhibit the
       * compiler from potentially optimizing away the whole of main()
       * on the grounds that some of the tests overflow! */
      volatile double in = make_double (t->in);
      uint64_t outbits = (uint64_t)(int64_t)in;
#else
      extern uint64_t arm_fp_d2lz(uint64_t);
      uint64_t outbits = arm_fp_d2lz(t->in);
#endif

      if (outbits != t->out)
	{
	  printf ("FAIL: d2lz(%016" PRIx64 ") -> %016" PRIx64
		  ", expected %016" PRIx64 "\n", t->in, outbits, t->out);
	  failed = true;
	}
    }

  if (!failed)
    printf ("all passed\n");

  return failed;
}
