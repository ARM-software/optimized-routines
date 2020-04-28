/*
 * memchr - find a character in a memory zone
 *
 * Copyright (c) 2014-2020, Arm Limited.
 * SPDX-License-Identifier: MIT
 */

/* Assumptions:
 *
 * ARMv8-a, AArch64
 * Neon Available.
 */

#include "../asmdefs.h"

/* Arguments and results.  */
#define srcin		x0
#define chrin		w1
#define cntin		x2

#define result		x0

#define src		x3
#define	tmp		x4
#define	tmp2		x5
#define wtmp2		w5
#define synd		x6
#define soff		x9
#define cntrem		x10

#define vrepchr		v0
#define qdata		q1
#define vdata		v1
#define vhas_chr	v2
#define vrepmask	v3
#define vend		v4

/*
 * Core algorithm:
 *
 * For each 16-byte chunk we calculate a 64-bit syndrome value, with four bits
 * per byte. For each tuple, bit 0 is set if the relevant byte matched the
 * requested character and bit 1, 2, 3 are not used (faster than using a lower
 * bit syndrome). Since the bits in the syndrome reflect exactly the order in
 * which things occur in the original string, counting trailing zeros allows to
 * identify exactly which byte has matched.
 */

ENTRY (__memchr_aarch64_mte)
	/* Do not dereference srcin if no bytes to compare.  */
	cbz	cntin, L(zero_length)
	/*
	 * Magic constant 0x10011001 allows us to identify which lane matches
	 * the requested byte.
	 */
	mov	wtmp2, #0x1001
	movk	wtmp2, #0x1001, lsl #16
	dup	vrepchr.16b, chrin
	/* Work with aligned 16-byte chunks */
	bic	src, srcin, #15
	dup	vrepmask.4s, wtmp2
	ands	soff, srcin, #15
	and	cntrem, cntin, #15
	b.eq	L(aligned_start)

	/*
	 * Input string is not 16-byte aligned. We calculate the syndrome
	 * value for the aligned 16 bytes block containing the first bytes
	 * and mask the irrelevant part.
	 */

	ld1	{vdata.16b}, [src], #16
	sub	tmp, soff, #16
	adds	cntin, cntin, tmp
	cmeq	vhas_chr.16b, vdata.16b, vrepchr.16b
	lsl	tmp, soff, #2
	mov	tmp2, #~0
	and	vhas_chr.16b, vhas_chr.16b, vrepmask.16b
	lsl	tmp, tmp2, tmp
	addp	vend.16b, vhas_chr.16b, vhas_chr.16b		/* 128->64 */
	mov	synd, vend.d[0]
	/* Clear the soff*4 lower bits */
	and	synd, synd, tmp
	/* The first block can also be the last */
	b.ls	L(masklast)
	/* Have we found something already? */
	cbnz	synd, L(tail)

L(aligned_start:)
	/* Make sure that it won't overread by a 16-byte chunk */
	add	tmp, cntin, #15
	tbnz	tmp, 4, L(loop32_2)

L(loop32):
	ld1	{vdata.16b}, [src], #16
	subs	cntin, cntin, #16
	cmeq	vhas_chr.16b, vdata.16b, vrepchr.16b
	addp	vend.16b, vhas_chr.16b, vhas_chr.16b		/* 128->64 */
	mov	synd, vend.d[0]
	cbnz	synd, L(end)

L(loop32_2):
	ld1	{vdata.16b}, [src], #16
	subs	cntin, cntin, #16
	cmeq	vhas_chr.16b, vdata.16b, vrepchr.16b
	b.ls	L(end)
	addp	vend.16b, vhas_chr.16b, vhas_chr.16b		/* 128->64 */
	mov	synd, vend.d[0]
	/* We haven't found the character, loop with 32 byte chunks */
	cbz	synd, L(loop32)

L(end):
	/* Termination condition found, let's calculate the syndrome value */
	and	vhas_chr.16b, vhas_chr.16b, vrepmask.16b
	addp	vend.16b, vhas_chr.16b, vhas_chr.16b		/* 128->64 */
	mov	synd, vend.d[0]
	/* Only do the clear for the last possible block */
	b.hs	L(tail)

L(masklast):
	/* Clear the (16 - ((cntrem + soff) % 16)) * 4 upper bits */
	add	tmp, cntrem, soff
	and	tmp, tmp, #15
	sub	tmp, tmp, #16
	neg	tmp, tmp, lsl #2
	lsl	synd, synd, tmp
	lsr	synd, synd, tmp

L(tail):
	/* Count the trailing zeros using bit reversing */
	rbit	synd, synd
	/* Compensate the last post-increment */
	sub	src, src, #16
	/* Check that we have found a character */
	cmp	synd, #0
	/* And count the leading zeros */
	clz	synd, synd
	/* Compute the potential result */
	add	result, src, synd, lsr #2
	/* Select result or NULL */
	csel	result, xzr, result, eq
	ret

L(zero_length):
	mov	result, #0
	ret

END (__memchr_aarch64_mte)

END_FILE
