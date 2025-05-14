/*
 * Tests of IEEE 754 uint32 to single-precision conversion
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
       * arithmetic, instead of calling arm_fp_ui2f. */
      float out = (float)t->in;
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
