/*
 * stpcpy test.
 *
 * Copyright (c) 2019-2020, Arm Limited.
 * SPDX-License-Identifier: MIT
 */

#define _GNU_SOURCE
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
  F(stpcpy)
#if __aarch64__
  F(__stpcpy_aarch64)
  F(__stpcpy_aarch64_mte)
# if __ARM_FEATURE_SVE
  F(__stpcpy_aarch64_sve)
# endif
#endif
  {0, 0}
  // clang-format on
};
#undef F

#define ALIGN 32
#define LEN 512
static char dbuf[LEN + 3 * ALIGN];
static char sbuf[LEN + 3 * ALIGN];
static char wbuf[LEN + 3 * ALIGN];

static void *
alignup (void *p)
{
  return (void *) (((uintptr_t) p + ALIGN - 1) & -ALIGN);
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
  if (len > LEN || dalign >= ALIGN || salign >= ALIGN)
    abort ();
  for (i = 0; i < len + ALIGN; i++)
    {
      src[i] = '?';
      want[i] = dst[i] = '*';
    }
  for (int i = 0; src + i < s; i++)
    src[i] = 0;
  for (int i = 1; i <= ALIGN; i++)
    s[len + i] = (len + salign) & 1 ? 1 : 0;
  for (i = 0; i < len; i++)
    s[i] = w[i] = 'a' + (i & 31);
  s[len] = w[len] = '\0';

  p = fun->fun (d, s);
  if (p != d + len)
    ERR ("%s (%p,..) returned %p expected %p\n", fun->name, d, p, d + len);

  for (i = 0; i < len + ALIGN; i++)
    {
      if (dst[i] != want[i])
	{
	  ERR ("%s (align %d, align %d, %d) failed\n",
	       fun->name, dalign, salign, len);
	  quoteat ("got", dst, len + ALIGN, i);
	  quoteat ("want", want, len + ALIGN, i);
	  break;
	}
    }
}

int
main (void)
{
  int r = 0;
  for (int i = 0; funtab[i].name; i++)
    {
      err_count = 0;
      for (int d = 0; d < ALIGN; d++)
	for (int s = 0; s < ALIGN; s++)
	  for (int n = 0; n < LEN; n++)
	    test (funtab + i, d, s, n);

      printf ("%s %s\n", err_count ? "FAIL" : "PASS", funtab[i].name);
      if (err_count)
	r = -1;
    }
  return r;
}
