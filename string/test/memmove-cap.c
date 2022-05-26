/*
 * capability-aware memmove test.
 *
 * Copyright (c) 2019-2022, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cheri.h>
#include "stringlib.h"
#include "stringtest.h"

#define F(x, mte) {#x, x, mte},

static const struct fun
{
  const char *name;
  void *(*fun) (void *, const void *, size_t);
  int test_mte;
} funtab[] = {
  // clang-format off
#if __aarch64__ && __CHERI__
  F(__memmove_aarch64, 1)
# if __ARM_NEON
  F(__memmove_aarch64_simd, 1)
# endif
#endif
  {0, 0, 0}
  // clang-format on
};
#undef F

#define A 32
#define LEN 512

static unsigned char wbuf[3 * LEN + A] __attribute__ ((aligned (16)));
static unsigned char dbuf[3 * LEN + A] __attribute__ ((aligned (16)));

static void *
alignup (void *p, int align)
{
  return (void *) (((uintptr_t) p + align - 1) & -align);
}

static void *
aligndown (void *p, int align)
{
  return (void *) (((uintptr_t) p) & -align);
}

static void
test (const struct fun *fun, int align, int diff, int len)
{
  unsigned char *src = dbuf + align + LEN;
  unsigned char *dst = src + diff;
  unsigned char *want = wbuf + align + LEN;
  unsigned int capsz = sizeof(void *__capability);
  int headbytes = (unsigned char *)alignup(src, 16) - src;
  int tailbytes = (src + len) - (unsigned char *)aligndown(src + len, 16);
  int numcaps = (len - headbytes - tailbytes) / capsz;

  if (headbytes > len) {
    headbytes = len;
    numcaps = 0;
    tailbytes = 0;
  }

  void *p;
  int i;

  if (err_count >= ERR_LIMIT)
    return;

  for (i = 0; i < len; ++i)
    dst[i] = '*';
  for (i = 0; i < headbytes; ++i)
    *(src + i) = *(want+i) = 'a' + i % 23;
  for (i = 0; i < numcaps; ++i)
    *((void*__capability*)(src + headbytes + capsz * i)) =
        *((void*__capability*)(want + headbytes + capsz * i)) =
	(void *__capability)(dbuf + i);
  for (i = 0; i < tailbytes; ++i)
    *(src + headbytes + capsz * numcaps + i) =
        *(want + headbytes + capsz * numcaps + i) =
	'a' + (i + headbytes) % 23;

  p = fun->fun (dst, src, len);

  if (p != dst)
    ERR ("%s(%p,..) returned %p\n", fun->name, dst, p);

  for (i = 0; i < len; i++)
    {
      if (dst[i] != want[i])
	{
	  ERR ("%s(align %d, diff %d, len %d) failed\n", fun->name, align, diff, len);
	  quoteat ("got", dst, len, i);
	  quoteat ("want", want, len, i);
	  break;
	}
    }
  for (i = 0; i < numcaps; i++)
    {
      if (cheri_tag_get(*((void*__capability*)(dst + headbytes + capsz * i))) !=
	  cheri_tag_get(dbuf))
	{
	  ERR ("%s(align %d, diff %d, len %d) (untagged %d) failed\n", fun->name, align, diff, len, i);
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
      for (int s = 0; s < A; s++)
        for (int l = 1; l < LEN; l++)
          for (int diff = -LEN; diff < LEN; diff += 16)
            {
              test (funtab + i, s, diff, l);
            }
      printf ("%s %s\n", err_count ? "FAIL" : "PASS", funtab[i].name);
      if (err_count)
	r = -1;
    }
  return r;
}
