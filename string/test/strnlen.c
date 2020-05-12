/*
 * strnlen test.
 *
 * Copyright (c) 2019-2020, Arm Limited.
 * SPDX-License-Identifier: MIT
 */

#define _POSIX_C_SOURCE 200809L

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
  size_t (*fun) (const char *s, size_t m);
} funtab[] = {
  // clang-format off
  F(strnlen)
#if __aarch64__
  F(__strnlen_aarch64)
# if __ARM_FEATURE_SVE
  F(__strnlen_aarch64_sve)
# endif
#endif
  {0, 0}
  // clang-format on
};
#undef F

#define ALIGN 32
#define LEN 512
static char sbuf[LEN + 3 * ALIGN];

static void *
alignup (void *p)
{
  return (void *) (((uintptr_t) p + ALIGN - 1) & -ALIGN);
}

static void
test (const struct fun *fun, int align, size_t maxlen, size_t len)
{
  char *src = alignup (sbuf);
  char *s = src + align;
  size_t r;
  size_t e = maxlen < len ? maxlen : len;

  if (err_count >= ERR_LIMIT)
    return;
  if (len > LEN || align > ALIGN)
    abort ();

  for (int i = 0; src + i < s; i++)
    src[i] = 0;
  for (int i = 1; i <= ALIGN; i++)
    s[len + i] = 0;
  for (int i = 0; i < len; i++)
    s[i] = 'a' + (i & 31);
  s[len] = 0;
  s[((len ^ align) & 1) ? e + 1 : len + 1] = 0;

  r = fun->fun (s, maxlen);
  if (r != e)
    {
      ERR ("%s (%p, %zu) len %zu returned %zu, expected %zu\n", fun->name, s,
	   maxlen, len, r, e);
      quote ("input", s, len);
    }
}

int
main (void)
{
  int r = 0;
  for (int i = 0; funtab[i].name; i++)
    {
      err_count = 0;
      for (int a = 0; a < ALIGN; a++)
	for (int n = 0; n < LEN; n++)
	  {
	    for (int maxlen = 0; maxlen < LEN; maxlen++)
	      test (funtab + i, a, maxlen, n);
	    test (funtab + i, a, SIZE_MAX - a, n);
	  }

      printf ("%s %s\n", err_count ? "FAIL" : "PASS", funtab[i].name);
      if (err_count)
	r = -1;
    }
  return r;
}
