/*
 * memcpy/memmove using SIMD registers
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

#define A_q	q0
#define B_q	q1
#define C_q	q2
#define D_q	q3
#define E_q	q4
#define F_q	q5
#define G_q	q6
#define H_q	q7

/* This implementation of memcpy correctly handles overlaps, therefore
   __memmove_aarch64_simd aliases to __memcpy_aarch64_simd. By moving the
   src and dst buffer overlap check from the start of memmove code to the
   beginning of large copy code, the overhead of combining memcpy
   and memmove implementations is negligible.

   Copies are split into 3 main cases: small copies of up to 16 bytes,
   medium copies of 17..128 bytes which are fully unrolled, and large
   copies (moves).

   Large forward moves align the source and use an unrolled loop
   processing 64 bytes per iteration.

   Large backward moves align srcend and use an unrolled loop processing
   64 bytes per iteration.
*/

ENTRY (__memcpy_aarch64_simd)
ENTRY_ALIAS (__memmove_aarch64_simd)
	add	srcend, src, count
	add	dstend, dstin, count
	cmp	count, 16
	b.ls	L(copy16_simd)
	cmp	count, 128
	b.hi	L(move_long_simd)

	/* Medium copies: 17..128 bytes.  */
	ldr	A_q, [src]
	ldr	D_q, [srcend, -16]
	cmp	count, 32
	b.hi	L(copy33_128_simd)
	str	A_q, [dstin]
	str	D_q, [dstend, -16]
	ret

	.p2align 4
	/* Small copies: 0..16 bytes.  */
L(copy16_simd):
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
L(copy33_128_simd):
	ldr	B_q, [src, 16]
	ldr	C_q, [srcend, -32]
	cmp	count, 64
	b.hi	L(copy65_128_simd)
	str	A_q, [dstin]
	str	D_q, [dstend, -16]
	str	B_q, [dstin, 16]
	str	C_q, [dstend, -32]
	ret

	.p2align 4
	/* Copy 65..128 bytes.  */
L(copy65_128_simd):
	ldr	E_q, [src, 32]
	ldr	F_q, [src, 48]
	ldr	G_q, [srcend, -64]
	ldr	H_q, [srcend, -48]
	str	A_q, [dstin]
	str	D_q, [dstend, -16]
	str	B_q, [dstin, 16]
	str	C_q, [dstend, -32]
	str	E_q, [dstin, 32]
	str	F_q, [dstin, 48]
	str	G_q, [dstend, -64]
	str	H_q, [dstend, -48]
	ret

	.p2align 4
	/* Move more than 128 bytes.  */
L(move_long_simd):
	sub	tmp1, dstin, src	/* Overlap check.  */
	cbz	tmp1, L(copy0_simd)
	cmp	tmp1, count
	b.lo	L(move_long_backwards_simd)

	/* Align src to 16 byte alignment so that we don't cross cache line
	   boundaries on both loads and stores.  There are at least 128 bytes
	   to copy, so copy 16 bytes unaligned and then align.  The loop
	   copies 64 bytes per iteration and prefetches one iteration ahead.  */

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
	b.ls	L(copy64_from_end_simd)

L(loop64_simd):
	str	A_q, [dst, 16]
	ldr	A_q, [src, 16]
	str	B_q, [dst, 32]
	ldr	B_q, [src, 32]
	str	C_q, [dst, 48]
	ldr	C_q, [src, 48]
	str	D_q, [dst, 64]!
	ldr	D_q, [src, 64]!
	subs	count, count, 64
	b.hi	L(loop64_simd)

	/* Write the last full set of 64 bytes.  The remainder is at most 64
	   bytes, so it is safe to always copy 64 bytes from the end even if
	   there is just 1 byte left.  */
L(copy64_from_end_simd):
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

L(copy0_simd):
	ret

	.p2align 4

	/* Move more than 128 bytes where src and dst buffers overlap
	   and dst > src.

     Align srcend to 16 byte alignment so that we don't cross cache line
	   boundaries on both loads and stores.  There are at least 128 bytes
	   to copy, so copy 16 bytes unaligned and then align.  The loop
	   copies 64 bytes per iteration and prefetches one iteration ahead.  */

L(move_long_backwards_simd):
	ldr	D_q, [srcend, -16]
	and	tmp1, srcend, 15
	sub	srcend, srcend, tmp1
	sub	count, count, tmp1
	ldr	A_q, [srcend, -16]
	str	D_q, [dstend, -16]
	ldr	B_q, [srcend, -32]
	ldr	C_q, [srcend, -48]
	ldr	D_q, [srcend, -64]!
	sub	dstend, dstend, tmp1
	subs	count, count, 128
	b.ls	L(copy64_from_start_simd)

L(loop64_backwards_simd):
	str	A_q, [dstend, -16]
	ldr	A_q, [srcend, -16]
	str	B_q, [dstend, -32]
	ldr	B_q, [srcend, -32]
	str	C_q, [dstend, -48]
	ldr	C_q, [srcend, -48]
	str	D_q, [dstend, -64]!
	ldr	D_q, [srcend, -64]!
	subs	count, count, 64
	b.hi	L(loop64_backwards_simd)

	/* Write the last full set of 64 bytes.  The remainder is at most 64
	   bytes, so it is safe to always copy 64 bytes from the start even if
	   there is just 1 byte left.  */
L(copy64_from_start_simd):
	ldr	G_q, [src, 48]
	str	A_q, [dstend, -16]
	ldr	A_q, [src, 32]
	str	B_q, [dstend, -32]
	ldr	B_q, [src, 16]
	str	C_q, [dstend, -48]
	ldr	C_q, [src]
	str	D_q, [dstend, -64]
	str	G_q, [dstin, 48]
	str	A_q, [dstin, 32]
	str	B_q, [dstin, 16]
	str	C_q, [dstin]
	ret

END (__memcpy_aarch64_simd)
