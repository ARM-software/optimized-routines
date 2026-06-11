/*
 * __arm_sc_memset - fill memory with a constant byte
 *
 * Copyright (c) 2012-2026, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

/* Assumptions:
 *
 * ARMv8-a, AArch64, unaligned accesses.
 *
 */

#include "asmdefs.h"

#define dstin	x0
#define val	x1
#define valw	w1
#define count	x2
#define dst	x3
#define dstend	x4
#define zva_val	x5
#define off	x3
#define dstend2	x5

/* This implementation is similar to __memset_aarch64 but avoids FP/AdvSIMD
   register instructions.  */

ENTRY_ALIAS (__memset_scalar_sc)
ENTRY (__memset_scalar)
	/* Replicate the fill byte across the last half of the word.  */
	and	valw, valw, 255
	orr	valw, valw, valw, lsl 8
	orr	valw, valw, valw, lsl 16

	cmp	count, 16
	b.lo	L(set_lt16)

	/* Replicate the fill byte across the rest of the word.  */
	orr	val, val, val, lsl 32

	add	dstend, dstin, count
	cmp	count, 64
	b.hi	L(set_gt64)

L(set_16_64):
	/* Calculate an interior store offset based on half the count, This will be
	  one of 0, 16 or 32 bytes. Then perform four overlapping stores.  */
	mov	off, 48
	and	off, off, count, lsr 1
	sub	dstend2, dstend, off

	stp	val, val, [dstin]
	add	off, dstin, off
	stp	val, val, [off]
	stp	val, val, [dstend2, -16]
	stp	val, val, [dstend, -16]
	ret

	.p2align 4
L(set_lt16):
	add	dstend, dstin, count
	cmp	count, 4
	b.lo	1f

	/* For sizes 4..15, salculate an interior offset based on the count. Then
	   perform four overlapping 32 bit stores.  */
	lsr	off, count, 3				/* off = count >> 3.  */
	sub	dstend2, dstend, off, lsl 2
	str	valw, [dstin]
	str	valw, [dstin, off, lsl 2]
	str	valw, [dstend2, -4]
	str	valw, [dstend, -4]
	ret

	/* Potentially overlapping stores for sizes 0..3. */
1:	cbz	count, 2f		/* Skip stores entirely for a zero-length memset.  */
	lsr	off, count, 1
	strb	valw, [dstin]
	strb	valw, [dstin, off]
	strb	valw, [dstend, -1]
2:	ret

	.p2align 4
L(set_gt64):
	bic	dst, dstin, 15		/* Align dst down to 16 bytes.  */
	cmp	count, 128
	b.hi	L(set_gt128)

	/* For sizes 64..128, perform eight overlapping 16-byte stores.  */
	stp	val, val, [dstin]
	stp	val, val, [dstin, 16]
	stp	val, val, [dstin, 32]
	stp	val, val, [dstin, 48]
	stp	val, val, [dstend, -64]
	stp	val, val, [dstend, -48]
	stp	val, val, [dstend, -32]
	stp	val, val, [dstend, -16]
	ret

	.p2align 4
L(set_gt128):
	/* For sizes above 128, perform initial stores before entering loop on
	   aligned addresses.  */
	stp	val, val, [dstin]
	stp	val, val, [dst, 16]

	/* We can use DC ZVA to zero memory when the fill byte is zero and the block
	   size reported by DCZID_EL0 is 64 bytes.  */
	cbnz valw, L(no_zva)
#ifndef SKIP_ZVA_CHECK
	mrs	zva_val, dczid_el0
	and	zva_val, zva_val, 31
	cmp	zva_val, 4
	b.ne	L(no_zva)
#endif
	/* Complete stores up to the first 64 byte aligned block.  */
	stp	val, val, [dst, 32]
	stp	val, val, [dst, 48]

	bic	dst, dstin, 63		/* Align dst down to 64 bytes.  */
	/* Compute count, accounting for the stores above and bias for the loop
	   exit.  */
	sub	count, dstend, dst
	sub	count, count, 64 + 64

	/* Write last bytes before entering ZVA loop.  */
	stp	val, val, [dstend, -64]
	stp	val, val, [dstend, -48]
	stp	val, val, [dstend, -32]
	stp	val, val, [dstend, -16]

	.p2align 4
L(zva64_loop):
	add	dst, dst, 64
	dc	zva, dst		/* Zero the 64-byte cache block containing dst.  */
	subs	count, count, 64
	b.hi	L(zva64_loop)
	ret

	.p2align 3
L(no_zva):
	/* Compute count, accounting for the two stores in set_gt128 and bias for
	   the loop exit.  */
	sub	count, dstend, dst
	sub	count, count, 64 + 32

L(no_zva_loop):
	/* Perform eight contiguous 16-byte stores per iteration.  */
	stp	val, val, [dst, 32]
	stp	val, val, [dst, 48]
	stp	val, val, [dst, 64]
	stp	val, val, [dst, 80]
	add	dst, dst, 64
	subs	count, count, 64
	b.hi	L(no_zva_loop)

	/* Perform final stores for the tail of the range, potentially overlapping
	   with previous ones.  */
	stp	val, val, [dstend, -64]
	stp	val, val, [dstend, -48]
	stp	val, val, [dstend, -32]
	stp	val, val, [dstend, -16]
	ret
END (__memset_scalar)
