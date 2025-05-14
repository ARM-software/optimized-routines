/*
 * Tests of IEEE 754 uint64 to single-precision conversion
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
  uint64_t in;
  uint32_t out;
};

static const struct test tests[] = {
  { 0x0000000000000000, 0x00000000 },
  { 0x0000000000000001, 0x3f800000 },
  { 0x0000000008000000, 0x4d000000 },
  { 0x0000000008000004, 0x4d000000 },
  { 0x0000000008000008, 0x4d000000 },
  { 0x000000000800000c, 0x4d000001 },
  { 0x0000000008000010, 0x4d000001 },
  { 0x0000000008000014, 0x4d000001 },
  { 0x0000000008000018, 0x4d000002 },
  { 0x000000000800001c, 0x4d000002 },
  { 0x0000082345000000, 0x55023450 },
  { 0x4000004000000001, 0x5e800001 },
  { 0x8000000000000000, 0x5f000000 },
  { 0x8000008000000000, 0x5f000000 },
  { 0xffffffffffffffff, 0x5f800000 },
};

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
       * arithmetic, instead of calling arm_fp_ul2f. */
      float out = (float)t->in;
      uint32_t outbits = unmake_float(out);
#else
      extern uint32_t arm_fp_ul2f(uint64_t);
      uint32_t outbits = arm_fp_ul2f(t->in);
#endif

      if (outbits != t->out)
	{
	  printf ("FAIL: ul2f(%016" PRIx64 ") -> %08" PRIx32
		  ", expected %08" PRIx32 "\n", t->in, outbits, t->out);
	  failed = true;
	}
    }

  if (!failed)
    printf ("all passed\n");

  return failed;
}
