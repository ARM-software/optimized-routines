/*
 * memchr - find a character in a memory zone
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

#ifndef MEMCHR_SVE2
# define MEMCHR_SVE2 __memchr_aarch64_sve2
#endif

/* Arguments and results.  */
#define srcin		x0
#define chrin		w1
#define cntin		x2
#define result		x0

#define negvlength	x3		/* Negative vector length.  */
#define pos		x4		/* Position of the first chr found.  */
#define srcend		x5		/* Address of the last byte.  */
#define srcendaligned	x6		/* Last address aligned to 2VL.  */
#define srcaligned	x7		/* Aligned src pointer.  */
#define tmp		x8		/* Temporary register.  */
#define nextsrcaligned	x9		/* Next aligned src pointer.  */

#define zdata1		z0		/* Data pointed by src.  */
#define zdata2		z1		/* Data pointed by src + 1VL.  */
#define zchr		z2		/* Chr data for search.  */
#define eorv1		z3		/* EOR between chr and zdata1.  */
#define eorv2		z4		/* EOR between chr and zdata2.  */

#define pall		p0		/* All active predicate.  */
#define pchr		p1		/* Chr search result.  */
#define pfirst		p2		/* First found.  */

ENTRY (MEMCHR_SVE2)
	cbz	cntin, L(no_result)	/* Do not dereference.  */
	dup	zchr.b, chrin		/* Initialize chr data for search.  */
	mov	negvlength, #0		/* Size of a vector.  */
	decb	negvlength

	and	tmp, srcin, 0xfff	/* Check for pagecross.  */
	incb	tmp, all, mul #2	/* Check for 2VL reads.  */
	tbnz	tmp, 12, L(pagecross)
L(start):
	ptrue	pall.b
	ldbsve	zdata1, [srcin]				/* Load.  */
	match	pchr.b, pall/z, zdata1.b, zchr.b	/* Find chr.  */
	b.any	L(firstmatch)			/* Move to firstmatch.  */
	adds	cntin, cntin, negvlength	/* Decrement cntin by 1VL.  */
	b.ls	L(no_result)
	ldbsve	zdata1, [srcin, 1, mul vl]	/* Read next chunk.  */
	sub	srcin, srcin, negvlength	/* Increment by 1VL.  */
	match	pchr.b, pall/z, zdata1.b, zchr.b	/* Find chr.  */
	b.none	L(cont)				/* If not found, continue.  */
L(firstmatch):
	brkb	pfirst.b, pall/z, pchr.b	/* Find first element.  */
	cntp	pos, pall, pfirst.b		/* Add index to pos.  */
	cmp	pos, cntin
	add	srcin, srcin, pos
	csel	result, srcin, xzr, lo		/* If pos > cntin ret null.  */
	ret

	.p2align 4
L(cont):
	cmn	cntin, negvlength
	b.ls	L(no_result)			/* Not found.  */
	add	srcend, srcin, cntin		/* Last src ptr.  */
	sub	srcin, srcin, negvlength	/* Move src ptr.  */
	and	srcendaligned, srcend, negvlength, lsl #1	/* Align.  */
	and	srcin, srcin, negvlength, lsl #1		/* Align.  */
	sub	cntin, srcend, srcin		/* Update cntin.  */
	cmp	srcin, srcendaligned		/* If less than 2VL.  */
	b.eq	L(start)			/* Not enough data.  */

	.p2align 4
L(loop):
	ldbsve	zdata1, [srcin]			/* Read 1st VL bytes.  */
	ldbsve	zdata2, [srcin, 1, mul vl]	/* Read next VL bytes.  */
	eor	eorv1.d,  zdata1.d, zchr.d	/* 0 if chrin is found.  */
	eor	eorv2.d,  zdata2.d, zchr.d	/* 0 if chrin is found.  */
	umin	eorv1.b, pall/m, eorv1.b, eorv2.b	/* Approximate idx.  */

	cmpeq	pchr.b, pall/z, eorv1.b, #0	/* Matched chrin?  */
	b.any	L(before_loop_end)		/* Find actual index.  */

	sub	srcin, srcin, negvlength, lsl #1	/* Add 2VL to src.  */
	cmp	srcin, srcendaligned
	b.ne	L(loop)

	sub	cntin, srcend, srcin		/* Update cntin.  */
	cbnz	cntin, L(start)
	b	L(no_result)

L(before_loop_end):
	cmpeq   pfirst.b, pall/z, zdata1.b, zchr.b	/* Chr in zdata1?  */
	b.none	L(loop_end)
	brkb	pfirst.b, pall/z, pfirst.b	/* Find first element.  */
	incp	srcin, pfirst.b			/* Add element index.  */
	ret
L(loop_end):
	sub	srcin, srcin, negvlength	/* Chr in zdata2.  */
	brkb	pfirst.b, pall/z, pchr.b	/* Find first element.  */
	incp	srcin, pfirst.b			/* Add element index.  */
	ret

	.p2align 4
L(pagecross):
	and	srcaligned, srcin, negvlength		/* Align src to VL.  */
	sub	nextsrcaligned, srcaligned, negvlength
	whilehi	pall.b, nextsrcaligned, srcin		/* True until src.  */
	ldbsve	zdata1, [srcaligned]			/* Aligned access.  */
	sub	tmp, nextsrcaligned, srcin
	match	pchr.b, pall/z, zdata1.b, zchr.b	/* Match valid chr.  */
	b.any	L(firstmatch)				/* Found? Jump.  */

	subs	cntin, cntin, tmp
	mov	srcin, nextsrcaligned		/* Next aligned address.  */
	b.hi	L(start)			/* Process 2VL chr.  */
L(no_result):
	mov	x0, #0				/* Not found.  */
	ret

END (MEMCHR_SVE2)
