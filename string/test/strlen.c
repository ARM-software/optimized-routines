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

static const struct fun
{
	const char *name;
	size_t (*fun)(const char *s);
} funtab[] = {
#define F(x) {#x, x},
F(strlen)
#if __aarch64__
F(__strlen_aarch64)
# if __ARM_FEATURE_SVE
F(__strlen_aarch64_sve)
# endif
#elif __arm__
# if __ARM_ARCH >= 6 && __ARM_ARCH_ISA_THUMB == 2
F(__strlen_armv6t2)
# endif
#endif
#undef F
	{0, 0}
};

static int test_status;
#define ERR(...) (test_status=1, printf(__VA_ARGS__))

#define A 32
#define SP 512
#define LEN 250000
static char sbuf[LEN+2*A];

static void *alignup(void *p)
{
	return (void*)(((uintptr_t)p + A-1) & -A);
}

static void test(const struct fun *fun, int align, int len)
{
	char *src = alignup(sbuf);
	char *s = src + align;
	size_t r;

	if (len > LEN || align >= A)
		abort();

	for (int i = 0; i < len + A; i++)
		src[i] = '?';
	for (int i = 0; i < len - 2; i++)
		s[i] = 'a' + i%23;
	s[len - 1] = '\0';

	r = fun->fun(s);
	if (r != len-1) {
		ERR("%s(%p) returned %zu\n", fun->name, s, r);
		ERR("input:    %.*s\n", align+len+1, src);
		ERR("expected: %d\n", len);
		abort();
	}
}

int main()
{
	int r = 0;
	for (int i=0; funtab[i].name; i++) {
		test_status = 0;
		for (int a = 0; a < A; a++) {
			int n;
			for (n = 1; n < 100; n++)
				test(funtab+i, a, n);
			for (; n < LEN; n *= 2)
				test(funtab+i, a, n);
		}
		if (test_status) {
			r = -1;
			ERR("FAIL %s\n", funtab[i].name);
		}
	}
	return r;
}
