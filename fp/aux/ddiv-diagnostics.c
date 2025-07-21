/*
 * Test cases for arm_fp_ddiv in DIAGNOSTICS mode
 *
 * Copyright (c) 1999-2025, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

/*
 * This small test program is compiled by fp/Dir.mk, together with a version of
 * fp/at32/ddiv.S compiled with -DDIAGNOSTICS. When run, it will call the
 * diagnostic version of arm_fp_ddiv with each of the pairs of input mantissas
 * listed in the tests[] array, so that the ddiv.S diagnostic macros will print
 * every detail of the calculation of the approximate quotient of those
 * mantissas.
 *
 * Each output diagnostic dump is preceded by a command line to ddiv-prove.py,
 * which passes the same pair of inputs to the reference Python implementation
 * of the same calculation in that script. This allows a cross-check that the
 * algorithm implemented in ddiv.S is the same algorithm that ddiv-prove.py's
 * error-bounding proof applies to.
 */

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

struct test
{
  uint64_t numerator, denominator;
};

static const struct test tests[] = {
    /*
     * We need a test both with and without renormalization.
     *
     * These two tests give the same numerator and denominator, but opposite
     * ways round, so that in one test a > b and in the other b > a.
     *
     * Other than that, the bits of the two inputs are simply chosen to be
     * random-looking enough that it's unlikely that the same output would
     * arise from doing two different computations on the input.
     */
    { 0xc34f0d52f2478800, 0xace0971c2073f800 },
    { 0xace0971c2073f800, 0xc34f0d52f2478800 },

    /*
     * One tricky part of the reciprocal calculation is the negation via
     * bitwise NOT (one's complement) of an intermediate result during the
     * final Newton-Raphson iteration. It was difficult to find a way to
     * express that in Gappa's input language which would allow it to prove a
     * useful error bound on it.
     *
     * The number being negated is the top 64 bits of a longer 96-bit value P
     * output from a multiplication. The one's complement negation of the top
     * 64 bits of P is the same as the top 64 bits of -P, in all cases except
     * when the bottom 32 bits of P are all zero. Therefore a case with that
     * property is included here, so that its intermediate values can be
     * checked.
     */
    { 0x8000000000000000, 0x89abcdef00000000 },
};

extern uint64_t arm_fp_ddiv(uint64_t, uint64_t);

int
main (void)
{
  for (size_t i = 0; i < sizeof (tests) / sizeof (tests[0]); i++)
    {
      const struct test *t = &tests[i];
      printf ("ddiv-prove.py --eval 0x%016" PRIx64 " 0x%016" PRIx64 "\n",
              t->numerator, t->denominator);
      arm_fp_ddiv(0x3ff0000000000000 | (t->numerator >> 11),
                  0x3ff0000000000000 | (t->denominator >> 11));
      printf ("\n");
    }

  return 0;
}
