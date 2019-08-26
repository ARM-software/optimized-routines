/*
 * memchr test.
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

static const struct fun
{
	const char *name;
	void *(*fun)(const void *, int c, size_t n);
} funtab[] = {
#define F(x) {#x, x},
F(memchr)
#if __aarch64__
F(__memchr_aarch64)
# if __ARM_FEATURE_SVE
F(__memchr_aarch64_sve)
# endif
#elif __arm__
F(__memchr_arm)
#endif
#undef F
	{0, 0}
};

static int test_status;
#define ERR(...) (test_status=1, printf(__VA_ARGS__))

#define A 32
#define SP 512
#define LEN 250000
static unsigned char sbuf[LEN+2*A];

static void *alignup(void *p)
{
	return (void*)(((uintptr_t)p + A-1) & -A);
}

static void test(const struct fun *fun, int align, int seekpos, int len)
{
	unsigned char *src = alignup(sbuf);
	unsigned char *s = src + align;
	unsigned char *f = len ? s + seekpos : 0;
	int seekchar = 0x1;
	int i;
	void *p;

	if (len > LEN || seekpos >= len || align >= A)
		abort();

	for (i = 0; i < seekpos; i++)
		s[i] = 'a' + i%23;
	s[i++] = seekchar;
	for (; i < len; i++)
		s[i] = 'a' + i%23;

	p = fun->fun(s, seekchar, len);

	if (p != f) {
		ERR("%s(%p,0x%02x,%d) returned %p\n", fun->name, s, seekchar, len, p);
		ERR("expected: %p\n", f);
		abort();
	}
}

int main()
{
	int r = 0;
	for (int i=0; funtab[i].name; i++) {
		test_status = 0;
		for (int a = 0; a < A; a++) {
			for (int n = 0; n < 100; n++)
				for (int sp = 0; sp < n-1; sp++)
					test(funtab+i, a, sp, n);
			for (int n = 100; n < LEN; n *= 2) {
				test(funtab+i, a, n-1, n);
				test(funtab+i, a, n/2, n);
			}
		}
		if (test_status) {
			r = -1;
			ERR("FAIL %s\n", funtab[i].name);
		}
	}
	return r;
}
