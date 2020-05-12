/*
 * strlen test.
 *
 * Copyright (c) 2019, Arm Limited.
 * SPDX-License-Identifier: MIT
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "stringlib.h"
#include "stringtest.h"

#define F(x) {#x, x},

static const struct fun
{
  const char *name;
  size_t (*fun) (const char *s);
} funtab[] = {
  // clang-format off
  F(strlen)
#if __aarch64__
  F(__strlen_aarch64)
  F(__strlen_aarch64_mte)
# if __ARM_FEATURE_SVE
  F(__strlen_aarch64_sve)
# endif
#elif __arm__
# if __ARM_ARCH >= 6 && __ARM_ARCH_ISA_THUMB == 2
  F(__strlen_armv6t2)
# endif
#endif
  {0, 0}
  // clang-format on
};
#undef F

#define A 32
#define SP 512
#define LEN 250000
static char sbuf[LEN + 2 * A + 1];

static void *
alignup (void *p)
{
  return (void *) (((uintptr_t) p + A - 1) & -A);
}

static void
test (const struct fun *fun, int align, int len)
{
  char *src = alignup (sbuf);
  char *s = src + align;
  size_t r;

  if (err_count >= ERR_LIMIT)
    return;
  if (len > LEN || align >= A)
    abort ();

  for (int i = 0; i < len + A; i++)
    src[i] = '?';
  for (int i = 0; i < len; i++)
    s[i] = 'a' + i % 23;
  s[len] = '\0';

  r = fun->fun (s);
  if (r != len)
    {
      ERR ("%s(%p) returned %zu\n", fun->name, s, r);
      quote ("input", src, len);
      printf ("expected: %d\n", len);
    }
}

int
main ()
{
  int r = 0;
  for (int i = 0; funtab[i].name; i++)
    {
      err_count = 0;
      for (int a = 0; a < A; a++)
	{
	  int n;
	  for (n = 1; n < 100; n++)
	    test (funtab + i, a, n);
	  for (; n < LEN; n *= 2)
	    test (funtab + i, a, n);
	}
      printf ("%s %s\n", err_count ? "FAIL" : "PASS", funtab[i].name);
      if (err_count)
	r = -1;
    }
  return r;
}
