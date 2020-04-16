/*
 * strnlen test.
 *
 * Copyright (c) 2019, Arm Limited.
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

static const struct fun
{
	const char *name;
	size_t (*fun)(const char *s, size_t m);
} funtab[] = {
#define F(x) {#x, x},
F(strnlen)
#if __aarch64__
F(__strnlen_aarch64)
# if __ARM_FEATURE_SVE
F(__strnlen_aarch64_sve)
# endif
#endif
#undef F
	{0, 0}
};

#define A 32
#define SP 512
#define LEN 250000
static char sbuf[LEN+2*A+1];

static void *alignup(void *p)
{
	return (void*)(((uintptr_t)p + A-1) & -A);
}

static void test(const struct fun *fun, int align, int maxlen, int len)
{
	char *src = alignup(sbuf);
	char *s = src + align;
	size_t r;
	size_t e = maxlen < len ? maxlen : len;

	if (err_count >= ERR_LIMIT)
		return;
	if (len > LEN || align >= A)
		abort();

	for (int i = 0; i < len + A; i++)
		src[i] = '?';
	for (int i = 0; i < len; i++)
		s[i] = 'a' + i%23;
	s[len] = '\0';

	r = fun->fun(s, maxlen);
	if (r != e) {
		ERR("%s(%p, %d) returned %zu\n", fun->name, s, maxlen, r);
		quote("input", src, len+A);
		printf("expected: %zu\n", e);
	}
}

int main()
{
	int r = 0;
	for (int i=0; funtab[i].name; i++) {
		err_count = 0;
		for (int a = 0; a < A; a++) {
			int n;
			for (n = 1; n < 100; n++)
				for (int maxlen = 0; maxlen < 100; maxlen++)
					test(funtab+i, a, maxlen, n);
			for (; n < LEN; n *= 2) {
				test(funtab+i, a, n*2, n);
				test(funtab+i, a, n, n);
				test(funtab+i, a, n/2, n);
			}
		}
		printf("%s %s\n", err_count ? "FAIL" : "PASS", funtab[i].name);
		if (err_count)
			r = -1;
	}
	return r;
}
