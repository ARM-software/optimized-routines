/*
 * memchr - find a character in a memory zone
 *
 * Copyright (c) 2014-2026, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

/* Assumptions:
 *
 * ARMv8-a, AArch64 only
 */

#include "asmdefs.h"

#define result		x0
#define src_in		x0
#define chr_in		w1
#define chr_repeated	x1
#define count_in	x2

#define src		x3
#define zeroones	x4
#define	data1		x5
#define	tmp1		x6
#define tmp2		x7
#define found1		x8
#define src_end		x9

#define REP8_01 0x0101010101010101
#define REP8_7f 0x7f7f7f7f7f7f7f7f

/* This function implements the hacker's delight algorithm for searching
   matching values. We find them in words by performing an XOR against the
   repeated searched char. Then, x being the xor result, we calculate
   (x - 0x0101010101010101) & ~(x | 0x7f7f7f7f7f7f7f7f). Words with a matching
   byte will produce a non-zero result. */

ENTRY_ALIAS (__memchr_scalar_sc)
ENTRY (__memchr_scalar)
	cbz	count_in, L(no_match)

	/* Replicate the needle byte across the rest of the word.  */
	and	chr_in, chr_in, 255
	orr	chr_in, chr_in, chr_in, lsl 8
	orr	chr_in, chr_in, chr_in, lsl 16
	orr	chr_repeated, chr_repeated, chr_repeated, lsl 32
	mov	zeroones, REP8_01

	bic	src, src_in, 7
	ldr	data1, [src], 8
#ifdef __AARCH64EB__
	rev	data1, data1
#endif
	/* Skip unused bytes before src_in.  */
	lsl	tmp1, src_in, 3
	lsr	tmp2, zeroones, tmp1
	lsr	data1, data1, tmp1

	eor	data1, data1, chr_repeated
	sub	tmp1, data1, tmp2
	orr	tmp2, data1, REP8_7f
	bic	found1, tmp1, tmp2
	cbz	found1, L(loop_start)

	/* Calculate the offset and return it if offset < count_in.  */
	rbit	found1, found1
	clz	tmp1, found1
	cmp	count_in, tmp1, lsr 3
	add	result, src_in, tmp1, lsr 3
	csel	result, result, xzr, hi
	ret

	.p2align 4
L(no_match):
	mov	result, 0
	ret

L(loop_start):
	/* Deal with count_in being so large that src_end comes before src_in
	   due to wraparound. When this happens, set src_end to SIZE_MAX so
	   that we do not reject results based on the address.  */
	adds	src_end, src_in, count_in
	csinv	src_end, src_end, xzr, cc

	.p2align 4
L(loop):
	cmp	src, src_end
	b.hs	L(no_match)
	ldr	data1, [src], 8
#ifdef __AARCH64EB__
	rev	data1, data1
#endif
	eor	data1, data1, chr_repeated
	sub	tmp1, data1, zeroones
	orr	tmp2, data1, REP8_7f
	bics	xzr, tmp1, tmp2
	b.eq	L(loop)

	/* Found a match - return the address if it is before src_end.  */
	bic	found1, tmp1, tmp2
	sub	src, src, 8
	rbit	found1, found1
	clz	tmp1, found1
	add	result, src, tmp1, lsr 3
	cmp	src_end, result
	csel	result, result, xzr, hi
	ret

END (__memchr_scalar)
