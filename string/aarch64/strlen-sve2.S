/*
 * strlen - calculate the length of a string.
 *
 * Copyright (c) 2026, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "asmdefs.h"

.arch armv9-a+sve2

/* Assumptions:
 *
 * AArch64
 * SVE2 Available.
 */

#define srcin		x0
#define result		x0

#define negalign	x1
#define src		x2
#define pgoffset	x3			/* Offset in page.  */
#define tmp		x4
#define off1		x5
#define off2		x6

#define zdata		z0
#define zdata2		z1

#define pall		p0
#define pnul		p1			/* Search vector with NUL.  */
#define pnul2		p2
#define p1stchunk	p3			/* Mask for first chunk.  */
#define pindex		p4			/* Mask for index of match.  */

ENTRY (__strlen_aarch64_sve2)
	ptrue	pall.b				/* Initialize all active.  */
	and	pgoffset, srcin, 0xfff
	incb	pgoffset, all, mul #2
	tbnz	pgoffset, 12, L(pagecross)

	ldbsve	zdata, [srcin]			/* Read	first chunk.  */
	ldbsve	zdata2, [srcin, 1, mul vl]	/* Read	next chunk.  */
	cmpeq	pnul2.b, pall/z, zdata2.b, 0	/* Find NUL.  */
	cset	tmp, none
	cmpeq	pnul.b, pall/z, zdata.b, 0
	csel	tmp, tmp, xzr, none
	cbnz	tmp, L(preloop)
	brkb	pnul.b, pall/z, pnul.b
	brkb	pnul2.b, pall/z, pnul2.b
	mov	off2, 0
	incb	off2
	cntp	off1, pall, pnul.b
	incp	off2, pnul2.b
	csel	result, off1, off2, any
	ret

	.p2align 4
	nop
L(preloop):
	mov	negalign, 0
	decb	negalign
	and	src, srcin, negalign, lsl 1	/* Align src to 2xVL.  */

	.p2align 4
L(loop):
	ldbsve	zdata, [src, 2, mul vl]
	ldbsve	zdata2, [src, 3, mul vl]
	sub	src, src, negalign, lsl 1	/* Increment src by 2xVL.  */
	umin	zdata2.b, pall/m, zdata2.b, zdata.b
	cmpeq	pnul.b, pall/z, zdata2.b, 0	/* Find NUL.  */
	b.none	L(loop)

	cmpeq	pindex.b, pall/z, zdata.b, 0	/* Find NUL.  */
	brkb	pnul.b, pall/z, pnul.b
	brkb	pindex.b, pall/z, pindex.b
	sub	off1, src, srcin
	sub	off2, off1, negalign
	incp	off1, pindex.b
	incp	off2, pnul.b
	csel	result, off1, off2, any
	ret

	.p2align 4
L(pagecross):
	mov	negalign, 0
	decb	negalign
	and	src, srcin, negalign		/* Align src to VL.  */
	sub	tmp, src, negalign
	whilehi	p1stchunk.b, tmp, srcin		/* Mask for first chunk.  */
	ldbsve	zdata, [src]			/* Read	first chunk.  */
	cmpeq	pnul.b, p1stchunk/z, zdata.b, 0	/* Find NUL.  */
	b.none	L(2ndchunk)			/* Next chunk if not found.  */

	brkb	pindex.b, p1stchunk/z, pnul.b	/* Find first element.  */
	cntp	result, pall, pindex.b
	ret

L(2ndchunk):
	ldbsve	zdata, [src, 1, mul vl]		/* Read second chunk.  */
	and	src, srcin, negalign, lsl 1	/* Align src to 2xVL.  */
	cmpeq	pnul.b, pall/z, zdata.b, 0	/* Find NUL.  */
	b.none	L(loop)				/* Loop if not found.  */

	sub	result, tmp, srcin		/* Length until current src.  */
	brkb	pindex.b, pall/z, pnul.b	/* Find first element.  */
	incp	result, pindex.b		/* Add index.  */
	ret
END (__strlen_aarch64_sve2)
