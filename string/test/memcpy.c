/*
 * memcpy test.
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

static const struct fun
{
	const char *name;
	void *(*fun)(void *, const void *, size_t);
} funtab[] = {
#define F(x) {#x, x},
F(memcpy)
#if __aarch64__
F(__memcpy_aarch64)
# if __ARM_NEON
F(__memcpy_aarch64_simd)
# endif
#elif __arm__
F(__memcpy_arm)
#endif
#undef F
	{0, 0}
};

#define A 32
#define LEN 250000
static unsigned char dbuf[LEN+2*A];
static unsigned char sbuf[LEN+2*A];
static unsigned char wbuf[LEN+2*A];

static void *alignup(void *p)
{
	return (void*)(((uintptr_t)p + A-1) & -A);
}

static void test(const struct fun *fun, int dalign, int salign, int len)
{
	unsigned char *src = alignup(sbuf);
	unsigned char *dst = alignup(dbuf);
	unsigned char *want = wbuf;
	unsigned char *s = src + salign;
	unsigned char *d = dst + dalign;
	unsigned char *w = want + dalign;
	void *p;
	int i;

	if (err_count >= ERR_LIMIT)
		return;
	if (len > LEN || dalign >= A || salign >= A)
		abort();
	for (i = 0; i < len+A; i++) {
		src[i] = '?';
		want[i] = dst[i] = '*';
	}
	for (i = 0; i < len; i++)
		s[i] = w[i] = 'a' + i%23;

	p = fun->fun(d, s, len);
	if (p != d)
		ERR("%s(%p,..) returned %p\n", fun->name, d, p);
	for (i = 0; i < len+A; i++) {
		if (dst[i] != want[i]) {
			ERR("%s(align %d, align %d, %d) failed\n", fun->name, dalign, salign, len);
			quoteat("got", dst, len+A, i);
			quoteat("want", want, len+A, i);
			break;
		}
	}
}

int main()
{
	int r = 0;
	for (int i=0; funtab[i].name; i++) {
		err_count = 0;
		for (int d = 0; d < A; d++)
			for (int s = 0; s < A; s++) {
				int n;
				for (n = 0; n < 100; n++)
					test(funtab+i, d, s, n);
				for (; n < LEN; n *= 2)
					test(funtab+i, d, s, n);
			}
		printf("%s %s\n", err_count ? "FAIL" : "PASS", funtab[i].name);
		if (err_count)
			r = -1;
	}
	return r;
}
