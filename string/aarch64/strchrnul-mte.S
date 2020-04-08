/*
 * strchrnul - find a character or nul in a string
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

#define result		x0

#define src		x2
#define	tmp1		x3
#define wtmp2		w4
#define tmp3		x5

#define vrepchr		v0
#define qdata		q1
#define vdata		v1
#define vhas_nul	v2
#define vhas_chr	v3
#define vrepmask_c	v5
#define vend		v6

/* Core algorithm:

   For each 16-byte chunk we calculate a 64-bit syndrome value, with four bits
   per byte. For each tuple, bit 0 is set if the relevant byte matched the
   requested character or NUL and bit 1, 2, 3 are not used (faster than using
   a lower bit syndrome). Since the bits in the syndrome reflect exactly the
   order in which things occur in the original string, counting trailing zeros
   allows to identify exactly which byte has matched.  */

/* Locals and temporaries.  */

ENTRY (__strchrnul_aarch64_mte)
	/* Magic constant 0x10011001 to allow us to identify which lane
	   matches the requested byte.  */
	mov	wtmp2, #0x1001
	movk	wtmp2, #0x1001, lsl #16
	dup	vrepchr.16b, chrin
	bic	src, srcin, #15		/* Work with aligned 16-byte chunks. */
	dup	vrepmask_c.4s, wtmp2
	ands	tmp1, srcin, #15
	b.eq	L(loop)

	/* Input string is not 16-byte aligned.  Rather than forcing
	   the padding bytes to a safe value, we calculate the syndrome
	   for all the bytes, but then mask off those bits of the
	   syndrome that are related to the padding.  */
	ld1	{vdata.16b}, [src], #16
	cmeq	vhas_chr.16b, vdata.16b, vrepchr.16b
	cmeq	vhas_nul.16b, vdata.16b, #0
	lsl	tmp1, tmp1, #2
	orr	vhas_chr.16b, vhas_nul.16b, vhas_chr.16b
	mov	tmp3, #~0
	and	vend.16b, vhas_chr.16b, vrepmask_c.16b

	addp	vend.16b, vend.16b, vend.16b		/* 128->64 */
	lsl	tmp1, tmp3, tmp1

	mov	tmp3, vend.d[0]
	ands	tmp1, tmp3, tmp1	/* Mask padding bits.  */
	b.ne	L(tail)

L(loop):
	ld1	{vdata.16b}, [src], #16
	cmeq	vhas_nul.16b, vdata.16b, #0
	cmeq	vhas_chr.16b, vdata.16b, vrepchr.16b
	/* Use a fast check for the termination condition.  */
	orr	vhas_chr.16b, vhas_nul.16b, vhas_chr.16b
	addp	vend.16b, vhas_chr.16b, vhas_chr.16b		/* 128->64 */
	mov	tmp1, vend.d[0]
	cbz	tmp1, L(loop)

	/* Termination condition found, let's calculate the syndrome value */
	and	vend.16b, vhas_chr.16b, vrepmask_c.16b
	addp	vend.16b, vend.16b, vend.16b		/* 128->64 */

	mov	tmp1, vend.d[0]
L(tail):
	/* Count the trailing zeros, by bit reversing...  */
	rbit	tmp1, tmp1
	/* Re-bias source.  */
	sub	src, src, #16
	clz	tmp1, tmp1	/* And counting the leading zeros.  */
	add	result, src, tmp1, lsr #2
	ret

END (__strchrnul_aarch64_mte)
