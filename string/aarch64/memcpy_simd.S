/*
 * memcpy version using SIMD registers
 *
 * Copyright (c) 2019, Arm Limited.
 * SPDX-License-Identifier: MIT
 */

/* Assumptions:
 *
 * ARMv8-a, AArch64, unaligned accesses.
 *
 */

#include "../asmdefs.h"

#define dstin	x0
#define src	x1
#define count	x2
#define dst	x3
#define srcend	x4
#define dstend	x5
#define A_x	x6
#define B_x	x7
#define A_w	w6
#define B_w	w7
#define C_w	w8
#define tmp1	x14
#define A_q	q0
#define B_q	q1
#define C_q	q2
#define D_q	q3
#define E_q	q4
#define F_q	q5
#define G_q	q6
#define H_q	q7

/* Copies are split into 3 main cases: small copies of up to 16 bytes,
   medium copies of 17..128 bytes which are fully unrolled. Large copies
   of more than 128 bytes align the source and use an unrolled loop
   processing 64 bytes per iteration.
*/

ENTRY (__memcpy_aarch64_simd)
	add	srcend, src, count
	add	dstend, dstin, count
	cmp	count, 16
	b.ls	L(copy16)
	cmp	count, 128
	b.hi	L(copy_long_simd)

	/* Medium copies: 17..128 bytes.  */
	ldr	A_q, [src]
	ldr	D_q, [srcend, -16]
	cmp	count, 32
	b.hi	L(copy33_128)
	str	A_q, [dstin]
	str	D_q, [dstend, -16]
	ret

	.p2align 4
	/* Small copies: 0..16 bytes.  */
L(copy16):
	/* 8-16 bytes.  */
	cmp	count, 8
	b.lo	1f
	ldr	A_x, [src]
	ldr	B_x, [srcend, -8]
	str	A_x, [dstin]
	str	B_x, [dstend, -8]
	ret

	.p2align 4
1:
	/* 4-7 bytes.  */
	tbz	count, 2, 1f
	ldr	A_w, [src]
	ldr	B_w, [srcend, -4]
	str	A_w, [dstin]
	str	B_w, [dstend, -4]
	ret

	.p2align 4
	/* Copy 0..3 bytes.  Use a branchless sequence that copies the same
	   byte 3 times if count==1, or the 2nd byte twice if count==2.  */
1:
	cbz	count, 2f
	lsr	tmp1, count, 1
	ldrb	A_w, [src]
	ldrb	B_w, [srcend, -1]
	ldrb	C_w, [src, tmp1]
	strb	A_w, [dstin]
	strb	C_w, [dstin, tmp1]
	strb	B_w, [dstend, -1]
2:	ret

	.p2align 4
	/* Copy 33..128 bytes.  */
L(copy33_128):
	str	A_q, [dstin]
	str	D_q, [dstend, -16]
	ldr	B_q, [src, 16]
	ldr	C_q, [srcend, -32]
	cmp	count, 64
	b.hi	L(copy65_128)
	str	B_q, [dstin, 16]
	str	C_q, [dstend, -32]
	ret

	.p2align 4
	/* Copy 65..128 bytes.  */
L(copy65_128):
	ldr	E_q, [src, 32]
	ldr	F_q, [src, 48]
	str	B_q, [dstin, 16]
	str	C_q, [dstend, -32]
	ldr	G_q, [srcend, -64]
	ldr	H_q, [srcend, -48]
	str	E_q, [dstin, 32]
	str	F_q, [dstin, 48]
	str	G_q, [dstend, -64]
	str	H_q, [dstend, -48]
	ret

	/* Align SRC to 16 byte alignment so that we don't cross cache line
	   boundaries on both loads and stores.  There are at least 128 bytes
	   to copy, so copy 16 bytes unaligned and then align.  The loop
	   copies 64 bytes per iteration and prefetches one iteration ahead.  */

	.p2align 4
L(copy_long_simd):
	ldr	D_q, [src]
	and	tmp1, src, 15
	bic	src, src, 15
	sub	dst, dstin, tmp1
	add	count, count, tmp1	/* Count is now 16 too large.  */
	ldr	A_q, [src, 16]
	str	D_q, [dstin]
	ldr	B_q, [src, 32]
	ldr	C_q, [src, 48]
	ldr	D_q, [src, 64]!
	subs	count, count, 128 + 16	/* Test and readjust count.  */
	b.ls	2f

1:
	str	A_q, [dst, 16]
	ldr	A_q, [src, 16]
	str	B_q, [dst, 32]
	ldr	B_q, [src, 32]
	str	C_q, [dst, 48]
	ldr	C_q, [src, 48]
	str	D_q, [dst, 64]!
	ldr	D_q, [src, 64]!
	subs	count, count, 64
	b.hi	1b

	/* Write the last full set of 64 bytes.  The remainder is at most 64
	   bytes, so it is safe to always copy 64 bytes from the end even if
	   there is just 1 byte left.  */
2:
	ldr	E_q, [srcend, -64]
	str	A_q, [dst, 16]
	ldr	A_q, [srcend, -48]
	str	B_q, [dst, 32]
	ldr	B_q, [srcend, -32]
	str	C_q, [dst, 48]
	ldr	C_q, [srcend, -16]
	str	D_q, [dst, 64]
	str	E_q, [dstend, -64]
	str	A_q, [dstend, -48]
	str	B_q, [dstend, -32]
	str	C_q, [dstend, -16]
	ret

END (__memcpy_aarch64_simd)
