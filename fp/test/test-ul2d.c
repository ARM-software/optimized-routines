/*
 * Tests of IEEE 754 uint64 to double-precision conversion
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
  { 0x0000000000000000, 0x0000000000000000 },
  { 0x0000000000000001, 0x3ff0000000000000 },
  { 0x0000000080000000, 0x41e0000000000000 },
  { 0x0000000080000001, 0x41e0000000200000 },
  { 0x0000000080000003, 0x41e0000000600000 },
  { 0x0000000080000007, 0x41e0000000e00000 },
  { 0x00000000fffffff8, 0x41efffffff000000 },
  { 0x00000000fffffffc, 0x41efffffff800000 },
  { 0x00000000fffffffe, 0x41efffffffc00000 },
  { 0x00000000ffffffff, 0x41efffffffe00000 },
  { 0x0000082345670000, 0x42a0468ace000000 },
  { 0x0013d0942cab7317, 0x4333d0942cab7317 },
  { 0x0100000000000000, 0x4370000000000000 },
  { 0x0100000000000004, 0x4370000000000000 },
  { 0x0100000000000008, 0x4370000000000000 },
  { 0x010000000000000c, 0x4370000000000001 },
  { 0x0100000000000010, 0x4370000000000001 },
  { 0x0100000000000014, 0x4370000000000001 },
  { 0x0100000000000018, 0x4370000000000002 },
  { 0x010000000000001c, 0x4370000000000002 },
  { 0xffffffffffffffff, 0x43f0000000000000 },
};

uint64_t
unmake_double (double x)
{
  uint64_t r;
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
       * arithmetic, instead of calling arm_fp_ul2d. */
      double out = (double)t->in;
      uint64_t outbits = unmake_double(out);
#else
      extern uint64_t arm_fp_ul2d(uint64_t);
      uint64_t outbits = arm_fp_ul2d(t->in);
#endif

      if (outbits != t->out)
	{
	  printf ("FAIL: ul2d(%016" PRIx64 ") -> %016" PRIx64
		  ", expected %016" PRIx64 "\n", t->in, outbits, t->out);
	  failed = true;
	}
    }

  if (!failed)
    printf ("all passed\n");

  return failed;
}
