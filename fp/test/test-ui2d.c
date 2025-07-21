/*
 * Tests of IEEE 754 uint32 to double-precision conversion
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
  { 0x00000000, 0x0000000000000000 },
  { 0x00000001, 0x3ff0000000000000 },
  { 0x80000400, 0x41e0000080000000 },
  { 0x80000800, 0x41e0000100000000 },
  { 0xffffffff, 0x41efffffffe00000 },
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
       * arithmetic, instead of calling arm_fp_ui2d. */
      double out = (double)t->in;
      uint64_t outbits = unmake_double(out);
#else
      extern uint64_t arm_fp_ui2d(uint32_t);
      uint64_t outbits = arm_fp_ui2d(t->in);
#endif

      if (outbits != t->out)
	{
	  printf ("FAIL: ui2d(%08" PRIx32 ") -> %016" PRIx64
		  ", expected %016" PRIx64 "\n", t->in, outbits, t->out);
	  failed = true;
	}
    }

  if (!failed)
    printf ("all passed\n");

  return failed;
}
