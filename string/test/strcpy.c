/*
 * strcpy test.
 *
 * Copyright (c) 2019-2020, Arm Limited.
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
  char *(*fun) (char *dest, const char *src);
} funtab[] = {
  // clang-format off
  F(strcpy)
#if __aarch64__
  F(__strcpy_aarch64)
  F(__strcpy_aarch64_mte)
# if __ARM_FEATURE_SVE
  F(__strcpy_aarch64_sve)
# endif
#elif __arm__ && defined (__thumb2__) && !defined (__thumb__)
  F(__strcpy_arm)
#endif
  {0, 0}
  // clang-format on
};
#undef F

#define A 32
#define LEN 250000
static char dbuf[LEN + 2 * A + 1];
static char sbuf[LEN + 2 * A + 1];
static char wbuf[LEN + 2 * A + 1];

static void *
alignup (void *p)
{
  return (void *) (((uintptr_t) p + A - 1) & -A);
}

static void
test (const struct fun *fun, int dalign, int salign, int len)
{
  char *src = alignup (sbuf);
  char *dst = alignup (dbuf);
  char *want = wbuf;
  char *s = src + salign;
  char *d = dst + dalign;
  char *w = want + dalign;
  void *p;
  int i;

  if (err_count >= ERR_LIMIT)
    return;
  if (len > LEN || dalign >= A || salign >= A)
    abort ();
  for (i = 0; i < len + A; i++)
    {
      src[i] = '?';
      want[i] = dst[i] = '*';
    }
  for (i = 0; i < len; i++)
    s[i] = w[i] = 'a' + i % 23;
  s[len] = w[len] = '\0';

  p = fun->fun (d, s);
  if (p != d)
    ERR ("%s(%p,..) returned %p\n", fun->name, d, p);
  for (i = 0; i < len + A; i++)
    {
      if (dst[i] != want[i])
	{
	  ERR ("%s(align %d, align %d, %d) failed\n", fun->name, dalign, salign,
	       len);
	  quoteat ("got", dst, len + A, i);
	  quoteat ("want", want, len + A, i);
	  break;
	}
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
	    for (n = 0; n < 100; n++)
	      test (funtab + i, d, s, n);
	    for (; n < LEN; n *= 2)
	      test (funtab + i, d, s, n);
	  }
      printf ("%s %s\n", err_count ? "FAIL" : "PASS", funtab[i].name);
      if (err_count)
	r = -1;
    }
  return r;
}
