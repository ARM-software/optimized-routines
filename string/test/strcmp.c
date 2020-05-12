/*
 * strcmp test.
 *
 * Copyright (c) 2019, Arm Limited.
 * SPDX-License-Identifier: MIT
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "stringlib.h"
#include "stringtest.h"

#define F(x) {#x, x},

static const struct fun
{
  const char *name;
  int (*fun) (const char *s1, const char *s2);
} funtab[] = {
  // clang-format off
  F(strcmp)
#if __aarch64__
  F(__strcmp_aarch64)
  F(__strcmp_aarch64_mte)
# if __ARM_FEATURE_SVE
  F(__strcmp_aarch64_sve)
# endif
#elif __arm__
# if __ARM_ARCH >= 7 && __ARM_ARCH_ISA_ARM >= 1
  F(__strcmp_arm)
# elif __ARM_ARCH == 6 && __ARM_ARCH_6M__ >= 1
  F(__strcmp_armv6m)
# endif
#endif
  {0, 0}
  // clang-format on
};
#undef F

#define A 32
#define LEN 250000
static char s1buf[LEN + 2 * A + 1];
static char s2buf[LEN + 2 * A + 1];

static void *
alignup (void *p)
{
  return (void *) (((uintptr_t) p + A - 1) & -A);
}

static void
test (const struct fun *fun, int s1align, int s2align, int len, int diffpos,
      int delta)
{
  char *src1 = alignup (s1buf);
  char *src2 = alignup (s2buf);
  char *s1 = src1 + s1align;
  char *s2 = src2 + s2align;
  int r;

  if (err_count >= ERR_LIMIT)
    return;
  if (len > LEN || s1align >= A || s2align >= A)
    abort ();
  if (diffpos >= len)
    abort ();
  if ((diffpos < 0) != (delta == 0))
    abort ();

  for (int i = 0; i < len + A; i++)
    src1[i] = src2[i] = '?';
  for (int i = 0; i < len; i++)
    s1[i] = s2[i] = 'a' + i % 23;
  if (delta)
    s1[diffpos] += delta;
  s1[len] = s2[len] = '\0';

  r = fun->fun (s1, s2);

  if ((delta == 0 && r != 0) || (delta > 0 && r <= 0) || (delta < 0 && r >= 0))
    {
      ERR ("%s(align %d, align %d, %d) failed, returned %d\n", fun->name,
	   s1align, s2align, len, r);
      quoteat ("src1", src1, len + A, diffpos);
      quoteat ("src2", src2, len + A, diffpos);
    }
}

int
main ()
{
  int r = 0;
  for (int i = 0; funtab[i].name; i++)
    {
      err_count = 0;
      for (int d = 0; d < A; d++)
	for (int s = 0; s < A; s++)
	  {
	    int n;
	    test (funtab + i, d, s, 0, -1, 0);
	    test (funtab + i, d, s, 1, -1, 0);
	    test (funtab + i, d, s, 1, 0, 1);
	    test (funtab + i, d, s, 1, 0, -1);
	    for (n = 2; n < 100; n++)
	      {
		test (funtab + i, d, s, n, -1, 0);
		test (funtab + i, d, s, n, n - 1, -1);
		test (funtab + i, d, s, n, n / 2, 1);
	      }
	    for (; n < LEN; n *= 2)
	      {
		test (funtab + i, d, s, n, -1, 0);
		test (funtab + i, d, s, n, n / 2, -1);
	      }
	  }
      printf ("%s %s\n", err_count ? "FAIL" : "PASS", funtab[i].name);
      if (err_count)
	r = -1;
    }
  return r;
}
