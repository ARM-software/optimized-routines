/*
 * Tests of IEEE 754 single-precision to double-precision conversion
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
  // Tests that don't depend on Arm-specific NaN policy
  { 0x00000001, 0x36a0000000000000 },
  { 0x00000003, 0x36b8000000000000 },
  { 0x00000005, 0x36c4000000000000 },
  { 0x00000009, 0x36d2000000000000 },
  { 0x00000011, 0x36e1000000000000 },
  { 0x00000021, 0x36f0800000000000 },
  { 0x00000041, 0x3700400000000000 },
  { 0x00000081, 0x3710200000000000 },
  { 0x00000101, 0x3720100000000000 },
  { 0x00000201, 0x3730080000000000 },
  { 0x00000401, 0x3740040000000000 },
  { 0x00000801, 0x3750020000000000 },
  { 0x00001001, 0x3760010000000000 },
  { 0x00002001, 0x3770008000000000 },
  { 0x00004001, 0x3780004000000000 },
  { 0x00008001, 0x3790002000000000 },
  { 0x00010001, 0x37a0001000000000 },
  { 0x00020001, 0x37b0000800000000 },
  { 0x00040001, 0x37c0000400000000 },
  { 0x00080001, 0x37d0000200000000 },
  { 0x00100001, 0x37e0000100000000 },
  { 0x00200001, 0x37f0000080000000 },
  { 0x00400001, 0x3800000040000000 },
  { 0x00800001, 0x3810000020000000 },
  { 0x01000001, 0x3820000020000000 },
  { 0x20000001, 0x3c00000020000000 },
  { 0x30000001, 0x3e00000020000000 },
  { 0x3f800000, 0x3ff0000000000000 },
  { 0x7f000000, 0x47e0000000000000 },
  { 0x7f7fffff, 0x47efffffe0000000 },
  { 0x7f800000, 0x7ff0000000000000 },
  { 0xff000000, 0xc7e0000000000000 },
  { 0xff7fffff, 0xc7efffffe0000000 },
  { 0xff800000, 0xfff0000000000000 },
  { 0x80800000, 0xb810000000000000 },
  { 0x807fffff, 0xb80fffffc0000000 },
  { 0x80400000, 0xb800000000000000 },
  { 0x803fffff, 0xb7ffffff80000000 },
  { 0x80000003, 0xb6b8000000000000 },
  { 0x80000002, 0xb6b0000000000000 },
  { 0x80000001, 0xb6a0000000000000 },
  { 0x80000000, 0x8000000000000000 },

  // Tests that do depend on Arm NaN policy
  { 0x7faf53b1, 0x7ffdea7620000000 },
  { 0x7fe111d3, 0x7ffc223a60000000 },
  { 0xffaf53b1, 0xfffdea7620000000 },
  { 0xffe111d3, 0xfffc223a60000000 },
};

float
make_float (uint32_t x)
{
  float r;
  memcpy (&r, &x, sizeof (r));
  return r;
}

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
       * arithmetic, instead of calling arm_fp_f2d. */
      double out = (double)make_float(t->in);
      uint64_t outbits = unmake_double(out);
#else
      extern uint64_t arm_fp_f2d(uint32_t);
      uint64_t outbits = arm_fp_f2d(t->in);
#endif

      if (outbits != t->out)
	{
	  printf ("FAIL: f2d(%08" PRIx32 ") -> %016" PRIx64
		  ", expected %016" PRIx64 "\n", t->in, outbits, t->out);
	  failed = true;
	}
    }

  if (!failed)
    printf ("all passed\n");

  return failed;
}
