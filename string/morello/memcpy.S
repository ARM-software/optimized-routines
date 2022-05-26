/*
 * memcpy - copy memory area
 *
 * Copyright (c) 2019-2022, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

/* Assumptions:
 *
 * ARMv8-a, AArch64, Morello, unaligned accesses.
 *
 */

#include "../asmdefs.h"

#define xdstin	x0
#define xsrc	x1
#define count	x2
#define xdst	x3
#define xsrcend	x4
#define xdstend	x5
#define auoff   x14
#define cap_count   x15
#define tmp1	x16
#define tmp2	x17

#if defined(__CHERI_PURE_CAPABILITY__)
#define dstin	c0
#define src	c1
#define dst	c3
#define srcend	c4
#define dstend	c5
#define tmp1_ptr c16
#else
#define dstin	x0
#define src	x1
#define dst	x3
#define srcend	x4
#define dstend	x5
#define tmp1_ptr x16
#endif

#define A_l   x6
#define B_l   x7
#define C_l   x8
#define D_l   x9
#define E_l   x10
#define F_l   x11
#define G_l   x12
#define H_l   x13

#define A_cap   c6
#define B_cap   c7
#define C_cap   c8
#define D_cap   c9
#define E_cap   c10
#define F_cap   c11
#define G_cap   c12
#define H_cap   c13


/* This algorithm has not been benchmarked. It's derived
   from the base aarch64 one with small changes to account
   for copying tags.

   1. We're copying less than 16 bytes, so no capabilities.
      Use the traditional code path for these.
   2. src mod 16 != dst mode 16. We're not copying capabilities,
      so again use the traditional memcpy.
   3. We're copying more than 8 capabilities plus the head and tail.
    a. No overlap, use forward copy
    b. Overlap, use backward copy
   4. We're copying 0..8 capabilities
    a. No capabilities to copy. This means we are copying 16..30 bytes.
       Use the existing code path to do this from the original algorithm.
    b. Copying 1..2 capabilities plus the head and tail
       Use a branchless sequence.
    c. Copying 3..4 capabilities plus the head and tail
       Use a branchless sequence.
    d. Copying 5..8 capabilities plus the head and tail
       Use a branchless sequence.
 */

ENTRY_ALIAS (__memmove_aarch64)
ENTRY (__memcpy_aarch64)
	PTR_ARG (0)
	PTR_ARG (1)
	SIZE_ARG (2)
	add	srcend, src, count
	add	dstend, dstin, count

	/* Copies of less than 16 bytes don't use capabilities. */
	cmp	count, 16
	b.lo	L(copy16)

	/* If src mod 16 != dst mod 16 we're not transfering tags. */
	and	tmp1, xsrc, 15
	and	tmp2, xdstin, 15
	cmp	tmp1, tmp2
	b.ne	L(memcpy_nocap)

	/* Get the number of capabilities that we need to store. */
	neg	tmp2, tmp1
	add	tmp2, tmp2, 16
	and	auoff, tmp2, 15

	sub	cap_count, count, auoff
	lsr	cap_count, cap_count, 4

	cmp	cap_count, 8
	b.hi	L(copy_long_cap)
	cmp	cap_count, 2
	b.hi	L(copy32_128_cap)

	/* Copy 0..2 capabilities using a branchless sequence. */
	cbz	cap_count, L(copy32)
	ldp	E_l, F_l, [src]
	ldp	C_l, D_l, [srcend, -16]
	add	src, src, auoff /* align up src to 16 bytes */
#if defined(__CHERI_PURE_CAPABILITY__)
	alignd	srcend, srcend, 4
#else
	bic	srcend, srcend, 15
#endif
	ldr	A_cap, [src]
	ldr	B_cap, [srcend, -16]
	stp	E_l, F_l, [dstin]
	stp	C_l, D_l, [dstend, -16]
	add	tmp1_ptr, dstin, auoff /* align up dstin to 16 bytes */
#if defined (__CHERI_PURE_CAPABILITY__)
	alignd	dstend, dstend, 4
#else
	bic	dstend, dstend, 15
#endif
	str	A_cap, [tmp1_ptr]
	str	B_cap, [dstend, -16]
	ret

	.p2align 4
L(copy32_128_cap):
	cmp	cap_count, 4
	b.hi	L(copy128_cap)
	/* Copy 3..4 capabilties using a branchless sequence. */
	ldp	E_l, F_l, [src]
	ldp	G_l, H_l, [srcend, -16]
	add	src, src, auoff /* align up src to 16 bytes */
#if defined (__CHERI_PURE_CAPABILITY__)
	alignd	srcend, srcend, 4
#else
	bic	srcend, srcend, 15
#endif
	ldp	A_cap, B_cap, [src]
	ldp	C_cap, D_cap, [srcend, -32]
	stp	E_l, F_l, [dstin]
	stp	G_l, H_l, [dstend, -16]
	add	tmp1_ptr, dstin, auoff /* align up dstin to 16 bytes */
#if defined (__CHERI_PURE_CAPABILITY__)
	alignd  dstend, dstend, 4
#else
	bic	dstend, dstend, 15
#endif
	stp	A_cap, B_cap, [tmp1_ptr]
	stp	C_cap, D_cap, [dstend, -32]
	ret

	.p2align 4
L(copy128_cap):
	/* Copy 5..8 capabilities using a branchless sequence. */
	ldp	count, tmp2, [src]
	ldp	tmp1, cap_count, [srcend, -16]
	add	src, src, auoff /* align up src to 16 bytes */
#if defined (__CHERI_PURE_CAPABILITY__)
	alignd	srcend, srcend, 4
#else
	bic	srcend, srcend, 15
#endif
	ldp	A_cap, B_cap, [src]
	ldp	C_cap, D_cap, [src, 32]
	ldp	E_cap, F_cap, [srcend, -32]
	ldp	G_cap, H_cap, [srcend, -64]
	stp	count, tmp2, [dstin]
	stp	tmp1, cap_count, [dstend, -16]
	add	tmp1_ptr, dstin, auoff /* align up src to 16 bytes */
#if defined (__CHERI_PURE_CAPABILITY__)
	alignd	dstend, dstend, 4
#else
	bic	dstend, dstend, 15
#endif
	stp	A_cap, B_cap, [tmp1_ptr]
	stp	C_cap, D_cap, [tmp1_ptr, 32]
	stp	E_cap, F_cap, [dstend, -32]
	stp	G_cap, H_cap, [dstend, -64]
	ret

L(copy_long_cap):
	/* Use backwards copy if there is an overlap.  */
	sub	tmp1, xdstin, xsrc
	cmp	tmp1, count
	b.lo	L(copy_long_backwards_cap)

	/* Copy 16 bytes and then align src to 16-byte alignment.  */
	ldp	C_l, D_l, [src]
	ldr	E_cap, [src, auoff]
	and	tmp1, xsrc, 15
#if defined(__CHERI_PURE_CAPABILITY__)
	alignd	src, src, 4
	neg	tmp2, tmp1
	add	dst, dstin, tmp2
#else
	bic	src, src, 15
	sub	dst, dstin, tmp1
#endif
	add	count, count, tmp1	/* Count is now 16 too large.  */
	ldp	A_cap, B_cap, [src, 16]
	stp	C_l, D_l, [dstin]
	str	E_cap, [dstin, auoff]
	ldp	C_cap, D_cap, [src, 48]
	subs	count, count, 128 + 16	/* Test and readjust count.  */
	b.ls	L(copy64_from_end_cap)
L(loop64_cap):
	stp	A_cap, B_cap, [dst, 16]
	ldp	A_cap, B_cap, [src, 80]
	stp	C_cap, D_cap, [dst, 48]
	ldp	C_cap, D_cap, [src, 112]
	add	src, src, 64
	add	dst, dst, 64
	subs	count, count, 64
	b.hi	L(loop64_cap)

	/* Write the last iteration and copy the last 16-byte aligned 64 byte block
	   from the end and the tail.  */
L(copy64_from_end_cap):
	ldp	G_l, H_l, [srcend, -16]
#if defined(__CHERI_PURE_CAPABILITY__)
	alignd	srcend, srcend, 4
	alignd	tmp1_ptr, dstend, 4
#else
	bic	srcend, srcend, 15
	bic	tmp1_ptr, dstend, 15
#endif
	ldp	E_cap, F_cap, [srcend, -64]
	stp	A_cap, B_cap, [dst, 16]
	ldp	A_cap, B_cap, [srcend, -32]
	stp	C_cap, D_cap, [dst, 48]
	stp	E_cap, F_cap, [tmp1_ptr, -64]
	stp	G_l, H_l, [dstend, -16]
	stp	A_cap, B_cap, [tmp1_ptr, -32]
	ret

L(copy_long_backwards_cap):
	cbz	tmp1, L(copy0)
	ldp	E_l, F_l, [srcend, -16]
	and	tmp1, xsrcend, 15
#if defined(__CHERI_PURE_CAPABILITY__)
	alignd	srcend, srcend, 4
	neg	tmp2, tmp1
	add	count, count, tmp2
#else
	bic	srcend, srcend, 15
	sub	count, count, tmp1
#endif
	ldp	A_cap, B_cap, [srcend, -32]
	stp	E_l, F_l, [dstend, -16]
	ldp	C_cap, D_cap, [srcend, -64]
#if defined(__CHERI_PURE_CAPABILITY__)
	add	dstend, dstend, tmp2  /* tmp1 was negated above to tmp2. */
#else
	sub	dstend, dstend, tmp1
#endif
	subs	count, count, 128
	b.ls	L(copy64_from_start)

L(loop64_backwards_cap):
	str	B_cap, [dstend, -16]
	str	A_cap, [dstend, -32]
	ldp	A_cap, B_cap, [srcend, -96]
	str	D_cap, [dstend, -48]
	str	C_cap, [dstend, -64]!
	ldp	C_cap, D_cap, [srcend, -128]
	sub	srcend, srcend, 64
	subs	count, count, 64
	b.hi	L(loop64_backwards_cap)

	/* Write the last iteration and copy 64 bytes from the start.  */
L(copy64_from_start_cap):
	ldp	G_l, H_l, [src]
	add	src, src, auoff /* align up src to 16 bytes */
	add	tmp1_ptr, dstin, auoff /* align up dstin to 16 bytes */
	ldp	E_cap, F_cap, [src, 32]
	stp	A_cap, B_cap, [dstend, -32]
	ldp	A_cap, B_cap, [src]
	stp	C_cap, D_cap, [dstend, -64]
	stp	E_cap, F_cap, [tmp1_ptr, 32]
	stp	G_l, H_l, [dstin]
	stp	A_cap, B_cap, [tmp1_ptr]
	ret

L(memcpy_nocap):
	cmp	count, 128
	b.hi	L(copy_long)
	cmp	count, 32
	b.hi	L(copy32_128)

#undef A_l
#undef B_l
#undef C_l
#undef D_l
#undef E_l
#undef F_l
#undef G_l
#undef H_l
#undef tmp1
#undef tmp1_ptr
#undef tmp2

#define A_l	x6
#define A_lw	w6
#define A_h	x7
#define B_l	x8
#define B_lw	w8
#define B_h	x9
#define C_l	x10
#define C_lw	w10
#define C_h	x11
#define D_l	x12
#define D_h	x13
#define E_l	x14
#define E_h	x15
#define F_l	x16
#define F_h	x17
#define G_l	count
#define G_h	xdst
#define H_l	xsrc
#define H_h	xsrcend
#define tmp1    E_l
#define tmp2    F_l

L(copy32):
	ldp	A_l, A_h, [src]
	ldp	D_l, D_h, [srcend, -16]
	stp	A_l, A_h, [dstin]
	stp	D_l, D_h, [dstend, -16]
	ret

	/* Copy 8-15 bytes.  */
L(copy16):
	tbz	count, 3, L(copy8)
	ldr	A_l, [src]
	ldr	A_h, [srcend, -8]
	str	A_l, [dstin]
	str	A_h, [dstend, -8]
	ret

	.p2align 3
	/* Copy 4-7 bytes.  */
L(copy8):
	tbz	count, 2, L(copy4)
	ldr	A_lw, [src]
	ldr	B_lw, [srcend, -4]
	str	A_lw, [dstin]
	str	B_lw, [dstend, -4]
	ret

	/* Copy 0..3 bytes using a branchless sequence.  */
L(copy4):
	cbz	count, L(copy0)
	lsr	tmp1, count, 1
	ldrb	A_lw, [src]
	ldrb	C_lw, [srcend, -1]
	ldrb	B_lw, [src, tmp1]
	strb	A_lw, [dstin]
	strb	B_lw, [dstin, tmp1]
	strb	C_lw, [dstend, -1]
L(copy0):
	ret

	.p2align 4
	/* Medium copies: 33..128 bytes.  */
L(copy32_128):
	ldp	A_l, A_h, [src]
	ldp	B_l, B_h, [src, 16]
	ldp	C_l, C_h, [srcend, -32]
	ldp	D_l, D_h, [srcend, -16]
	cmp	count, 64
	b.hi	L(copy128)
	stp	A_l, A_h, [dstin]
	stp	B_l, B_h, [dstin, 16]
	stp	C_l, C_h, [dstend, -32]
	stp	D_l, D_h, [dstend, -16]
	ret

	.p2align 4
	/* Copy 65..128 bytes.  */
L(copy128):
	ldp	E_l, E_h, [src, 32]
	ldp	F_l, F_h, [src, 48]
	cmp	count, 96
	b.ls	L(copy96)
	ldp	G_l, G_h, [srcend, -64]
	ldp	H_l, H_h, [srcend, -48]
	stp	G_l, G_h, [dstend, -64]
	stp	H_l, H_h, [dstend, -48]
L(copy96):
	stp	A_l, A_h, [dstin]
	stp	B_l, B_h, [dstin, 16]
	stp	E_l, E_h, [dstin, 32]
	stp	F_l, F_h, [dstin, 48]
	stp	C_l, C_h, [dstend, -32]
	stp	D_l, D_h, [dstend, -16]
	ret

	.p2align 4
	/* Copy more than 128 bytes.  */
L(copy_long):
	/* Use backwards copy if there is an overlap.  */
	sub	tmp1, xdstin, xsrc
	cbz	tmp1, L(copy0)
	cmp	tmp1, count
	b.lo	L(copy_long_backwards)

	/* Copy 16 bytes and then align dst to 16-byte alignment.  */

	ldp	D_l, D_h, [src]
	and	tmp1, xdstin, 15
#if defined(__CHERI_PURE_CAPABILITY__)
	alignd	dst, dstin, 4
	neg	tmp2, tmp1
	add	src, src, tmp2
#else
	bic	dst, dstin, 15
	sub	src, src, tmp1
#endif
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

	/* Write the last iteration and copy 64 bytes from the end.  */
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
	ret

	.p2align 4

	/* Large backwards copy for overlapping copies.
	   Copy 16 bytes and then align dst to 16-byte alignment.  */
L(copy_long_backwards):
	ldp	D_l, D_h, [srcend, -16]
	and	tmp1, xdstend, 15
#if defined(__CHERI_PURE_CAPABILITY__)
	neg	tmp2, tmp1
	add	srcend, srcend, tmp2
#else
	sub	srcend, srcend, tmp1
#endif
	sub	count, count, tmp1
	ldp	A_l, A_h, [srcend, -16]
	stp	D_l, D_h, [dstend, -16]
	ldp	B_l, B_h, [srcend, -32]
	ldp	C_l, C_h, [srcend, -48]
	ldp	D_l, D_h, [srcend, -64]!
#if defined(__CHERI_PURE_CAPABILITY__)
	add	dstend, dstend, tmp2
#else
	sub	dstend, dstend, tmp1
#endif
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

	/* Write the last iteration and copy 64 bytes from the start.  */
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
.size __memmove_aarch64, .-__memmove_aarch64
