/*
 * memcpy - copy memory area
 *
 * Copyright (c) 2012-2019, Arm Limited.
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
#define A_l	x6
#define A_lw	w6
#define A_h	x7
#define A_hw	w7
#define B_l	x8
#define B_lw	w8
#define B_h	x9
#define C_l	x10
#define C_h	x11
#define D_l	x12
#define D_h	x13
#define E_l	x14
#define E_h	x15
#define F_l	x16
#define F_h	x17
#define G_l	count
#define G_h	dst
#define H_l	src
#define H_h	srcend
#define tmp1	x14

/* This implementation of memcpy correctly handles overlaps, therefore
   __memmove_aarch64 aliases to __memcpy_aarch64. By moving the src and
   dst buffer overlap check from the start of memmove code to the
   beginning of large copy code, the overhead of combining memcpy
   and memmove implementations is negligible.

   Copies are split into 3 main cases: small copies of up to 16 bytes,
   medium copies of 17..128 bytes which are fully unrolled, and large
   copies (moves).

   Large forward moves align the destination and use an unrolled loop
   processing 64 bytes per iteration.

   Large backward moves align dstend and use an unrolled loop processing
   64 bytes per iteration.
*/

ENTRY (__memcpy_aarch64)
ENTRY_ALIAS (__memmove_aarch64)
	add	srcend, src, count
	add	dstend, dstin, count
	cmp	count, 16
	b.ls	L(copy16)
	cmp	count, 128
	b.hi	L(move_long)

	/* Medium copies: 17..128 bytes.  */
	ldp	A_l, A_h, [src]
	ldp	D_l, D_h, [srcend, -16]
	cmp	count, 32
	b.hi	L(copy33_128)
	stp	A_l, A_h, [dstin]
	stp	D_l, D_h, [dstend, -16]
	ret

	.p2align 4
	/* Small copies: 0..16 bytes.  */
L(copy16):
	/* 8-15 bytes.  */
	cmp	count, 8
	b.lo	1f
	ldr	A_l, [src]
	ldr	A_h, [srcend, -8]
	str	A_l, [dstin]
	str	A_h, [dstend, -8]
	ret

	.p2align 4
1:
	/* 4-7 bytes.  */
	tbz	count, 2, 1f
	ldr	A_lw, [src]
	ldr	A_hw, [srcend, -4]
	str	A_lw, [dstin]
	str	A_hw, [dstend, -4]
	ret

	.p2align 4
	/* Copy 0..3 bytes.  Use a branchless sequence that copies the same
	   byte 3 times if count==1, or the 2nd byte twice if count==2.  */
1:
	cbz	count, 2f
	lsr	tmp1, count, 1
	ldrb	A_lw, [src]
	ldrb	A_hw, [srcend, -1]
	ldrb	B_lw, [src, tmp1]
	strb	A_lw, [dstin]
	strb	B_lw, [dstin, tmp1]
	strb	A_hw, [dstend, -1]
2:	ret

	.p2align 4
	/* Copy 33..128 bytes.  */
L(copy33_128):
	ldp	B_l, B_h, [src, 16]
	ldp	C_l, C_h, [srcend, -32]
	cmp	count, 64
	b.hi	L(copy65_128)
	stp	A_l, A_h, [dstin]
	stp	D_l, D_h, [dstend, -16]
	stp	B_l, B_h, [dstin, 16]
	stp	C_l, C_h, [dstend, -32]
	ret

	.p2align 4
	/* Copy 65..128 bytes.  */
L(copy65_128):
	ldp	E_l, E_h, [src, 32]
	ldp	F_l, F_h, [src, 48]
	ldp	G_l, G_h, [srcend, -64]
	ldp	H_l, H_h, [srcend, -48]
	stp	A_l, A_h, [dstin]
	stp	D_l, D_h, [dstend, -16]
	stp	B_l, B_h, [dstin, 16]
	stp	C_l, C_h, [dstend, -32]
	stp	E_l, E_h, [dstin, 32]
	stp	F_l, F_h, [dstin, 48]
	stp	G_l, G_h, [dstend, -64]
	stp	H_l, H_h, [dstend, -48]
	ret

	.p2align 4
	/* Move more than 128 bytes.  */
L(move_long):
	sub	tmp1, dstin, src	/* Overlap check.  */
	cbz	tmp1, L(copy0)
	cmp	tmp1, count
	b.lo	L(move_long_backwards)

	/* Align dst to 16 byte alignment so that we don't cross cache line
	   boundaries on both loads and stores.  There are at least 128 bytes
	   to copy, so copy 16 bytes unaligned and then align.  The loop
	   copies 64 bytes per iteration and prefetches one iteration ahead.  */

	ldp	D_l, D_h, [src]
	and	tmp1, dstin, 15
	bic	dst, dstin, 15
	sub	src, src, tmp1
	add	count, count, tmp1	/* Count is now 16 too large.  */
	ldp	A_l, A_h, [src, 16]
	stp	D_l, D_h, [dstin]
	ldp	B_l, B_h, [src, 32]
	ldp	C_l, C_h, [src, 48]
	ldp	D_l, D_h, [src, 64]!
	subs	count, count, 128 + 16	/* Test and readjust count.  */
	b.ls	L(copy64_from_end)

L(loop64):
	stp	A_l, A_h, [dst, 16]
	ldp	A_l, A_h, [src, 16]
	stp	B_l, B_h, [dst, 32]
	ldp	B_l, B_h, [src, 32]
	stp	C_l, C_h, [dst, 48]
	ldp	C_l, C_h, [src, 48]
	stp	D_l, D_h, [dst, 64]!
	ldp	D_l, D_h, [src, 64]!
	subs	count, count, 64
	b.hi	L(loop64)

	/* Write the last full set of 64 bytes.  The remainder is at most 64
	   bytes, so it is safe to always copy 64 bytes from the end even if
	   there is just 1 byte left.  */
L(copy64_from_end):
	ldp	E_l, E_h, [srcend, -64]
	stp	A_l, A_h, [dst, 16]
	ldp	A_l, A_h, [srcend, -48]
	stp	B_l, B_h, [dst, 32]
	ldp	B_l, B_h, [srcend, -32]
	stp	C_l, C_h, [dst, 48]
	ldp	C_l, C_h, [srcend, -16]
	stp	D_l, D_h, [dst, 64]
	stp	E_l, E_h, [dstend, -64]
	stp	A_l, A_h, [dstend, -48]
	stp	B_l, B_h, [dstend, -32]
	stp	C_l, C_h, [dstend, -16]

L(copy0):
	ret

	.p2align 4

	/* Move more than 128 bytes where src and dst buffers overlap
	   and dst > src.

     Align dstend to 16 byte alignment so that we don't cross cache line
	   boundaries on both loads and stores.  There are at least 128 bytes
	   to copy, so copy 16 bytes unaligned and then align.  The loop
	   copies 64 bytes per iteration and prefetches one iteration ahead.  */
L(move_long_backwards):
	ldp	D_l, D_h, [srcend, -16]
	and	tmp1, dstend, 15
	sub	srcend, srcend, tmp1
	sub	count, count, tmp1
	ldp	A_l, A_h, [srcend, -16]
	stp	D_l, D_h, [dstend, -16]
	ldp	B_l, B_h, [srcend, -32]
	ldp	C_l, C_h, [srcend, -48]
	ldp	D_l, D_h, [srcend, -64]!
	sub	dstend, dstend, tmp1
	subs	count, count, 128
	b.ls	L(copy64_from_start)

L(loop64_backwards):
	stp	A_l, A_h, [dstend, -16]
	ldp	A_l, A_h, [srcend, -16]
	stp	B_l, B_h, [dstend, -32]
	ldp	B_l, B_h, [srcend, -32]
	stp	C_l, C_h, [dstend, -48]
	ldp	C_l, C_h, [srcend, -48]
	stp	D_l, D_h, [dstend, -64]!
	ldp	D_l, D_h, [srcend, -64]!
	subs	count, count, 64
	b.hi	L(loop64_backwards)

	/* Write the last full set of 64 bytes.  The remainder is at most 64
	   bytes, so it is safe to always copy 64 bytes from the start even if
	   there is just 1 byte left.  */
L(copy64_from_start):
	ldp	G_l, G_h, [src, 48]
	stp	A_l, A_h, [dstend, -16]
	ldp	A_l, A_h, [src, 32]
	stp	B_l, B_h, [dstend, -32]
	ldp	B_l, B_h, [src, 16]
	stp	C_l, C_h, [dstend, -48]
	ldp	C_l, C_h, [src]
	stp	D_l, D_h, [dstend, -64]
	stp	G_l, G_h, [dstin, 48]
	stp	A_l, A_h, [dstin, 32]
	stp	B_l, B_h, [dstin, 16]
	stp	C_l, C_h, [dstin]
	ret

END (__memcpy_aarch64)
