/*
 * memset - fill memory with a constant byte
 *
 * Copyright (c) 2012-2024, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

/* Assumptions:
 *
 * ARMv8-a, AArch64, Advanced SIMD, SVE, unaligned accesses.
 *
 */

#include "asmdefs.h"

#if HAVE_SVE

.arch armv8-a+sve

#define dstin	x0
#define val	x1
#define valw	w1
#define count	x2
#define dst	x3
#define dstend	x4
#define zva_val	x5
#define vlen	x5

ENTRY (__memset_aarch64_sve)
	PTR_ARG (0)
	SIZE_ARG (2)

	dup	z0.b, valw
	cmp	count, 128
	b.hs	L(set_long)

	cntb	vlen
	cmp	count, vlen, lsl 1
	b.hi	L(set_medium)

	whilelo p0.b, xzr, count
	whilelo p1.b, vlen, count
	st1b	z0.b, p0, [dstin, 0, mul vl]
	st1b	z0.b, p1, [dstin, 1, mul vl]
	ret

L(set_medium):
	add	dstend, dstin, count
	stp	q0, q0, [dstin]
	tbnz	count, 6, L(set128)
	stp	q0, q0, [dstend, -32]
	ret

	.p2align 4
L(set128):
	stp	q0, q0, [dstin, 32]
	stp	q0, q0, [dstend, -64]
	stp	q0, q0, [dstend, -32]
	ret

	.p2align 4
L(set_long):
	add	dstend, dstin, count
	and	valw, valw, 255
	bic	dst, dstin, 15
	str	q0, [dstin]
	cmp	count, 256
	ccmp	valw, 0, 0, hs
	b.ne	L(no_zva)

#ifndef SKIP_ZVA_CHECK
	mrs	zva_val, dczid_el0
	and	zva_val, zva_val, 31
	cmp	zva_val, 4		/* ZVA size is 64 bytes.  */
	b.ne	L(no_zva)
#endif
	str	q0, [dst, 16]
	stp	q0, q0, [dst, 32]
	bic	dst, dst, 63
	sub	count, dstend, dst	/* Count is now 64 too large.  */
	sub	count, count, 128	/* Adjust count and bias for loop.  */

	.p2align 4
L(zva_loop):
	add	dst, dst, 64
	dc	zva, dst
	subs	count, count, 64
	b.hi	L(zva_loop)
	stp	q0, q0, [dstend, -64]
	stp	q0, q0, [dstend, -32]
	ret

L(no_zva):
	sub	count, dstend, dst	/* Count is 16 too large.  */
	sub	dst, dst, 16		/* Dst is biased by -32.  */
	sub	count, count, 64 + 16	/* Adjust count and bias for loop.  */
L(no_zva_loop):
	stp	q0, q0, [dst, 32]
	stp	q0, q0, [dst, 64]!
	subs	count, count, 64
	b.hi	L(no_zva_loop)
	stp	q0, q0, [dstend, -64]
	stp	q0, q0, [dstend, -32]
	ret

END (__memset_aarch64_sve)

#endif
