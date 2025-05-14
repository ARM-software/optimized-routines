/*
 * Tests of IEEE 754 int32 to single-precision conversion
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
  { 0x00000000, 0x00000000 },
  { 0x00000001, 0x3f800000 },
  { 0x08000000, 0x4d000000 },
  { 0x08000004, 0x4d000000 },
  { 0x08000008, 0x4d000000 },
  { 0x0800000c, 0x4d000001 },
  { 0x08000010, 0x4d000001 },
  { 0x08000014, 0x4d000001 },
  { 0x08000018, 0x4d000002 },
  { 0x0800001c, 0x4d000002 },
  { 0x7fffffff, 0x4f000000 },
  { 0x80000000, 0xcf000000 },
  { 0x80000001, 0xcf000000 },
  { 0xf7ffffe4, 0xcd000002 },
  { 0xf7ffffe8, 0xcd000002 },
  { 0xf7ffffec, 0xcd000001 },
  { 0xf7fffff0, 0xcd000001 },
  { 0xf7fffff4, 0xcd000001 },
  { 0xf7fffff8, 0xcd000000 },
  { 0xf7fffffc, 0xcd000000 },
  { 0xf8000000, 0xcd000000 },
};

int32_t to_signed(uint32_t x)
{
  int32_t r;
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
       * arithmetic, instead of calling arm_fp_i2f. */
      float out = (float)to_signed(t->in);
      uint32_t outbits = unmake_float(out);
#else
      extern uint32_t arm_fp_i2f(uint32_t);
      uint32_t outbits = arm_fp_i2f(t->in);
#endif

      if (outbits != t->out)
	{
	  printf ("FAIL: i2f(%08" PRIx32 ") -> %08" PRIx32
		  ", expected %08" PRIx32 "\n", t->in, outbits, t->out);
	  failed = true;
	}
    }

  if (!failed)
    printf ("all passed\n");

  return failed;
}
