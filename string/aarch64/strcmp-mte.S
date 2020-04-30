/*
 * strcmp - compare two strings
 *
 * Copyright (c) 2012-2020, Arm Limited.
 * SPDX-License-Identifier: MIT
 */

/* Assumptions:
 *
 * ARMv8-a, AArch64
 */

#include "../asmdefs.h"

#define REP8_01 0x0101010101010101
#define REP8_7f 0x7f7f7f7f7f7f7f7f

/* Parameters and result.  */
#define src1		x0
#define src2		x1
#define result		x0

/* Internal variables.  */
#define data1		x2
#define data1w		w2
#define data2		x3
#define data2w		w3
#define has_nul		x4
#define diff		x5
#define syndrome	x6
#define tmp1		x7
#define tmp2		x8
#define tmp3		x9
#define zeroones	x10
#define pos		x11
#define offset		x12
#define neg_offset	x13
#define mask		x14

/* Define endian dependent shift operations.
   On big-endian early bytes are at MSB and on little-endian LSB.
   LS_FW means shifting towards early bytes.
   LS_BK means shifting towards later bytes.
   */
#ifdef __AARCH64EB__
#define LS_FW lsl
#define LS_BK lsr
#else
#define LS_FW lsr
#define LS_BK lsl
#endif

	/* Start of performance-critical section  -- one 64B cache line.  */
ENTRY (__strcmp_aarch64_mte)
	eor	tmp1, src1, src2
	mov	zeroones, #REP8_01
	tst	tmp1, #7
	b.ne	L(misaligned8)
	ands	tmp1, src1, #7
	b.ne	L(mutual_align)
	/* NUL detection works on the principle that (X - 1) & (~X) & 0x80
	   (=> (X - 1) & ~(X | 0x7f)) is non-zero iff a byte is zero, and
	   can be done in parallel across the entire word.  */
L(loop_aligned):
	ldr	data1, [src1], #8
	ldr	data2, [src2], #8
L(start_realigned):
	sub	tmp1, data1, zeroones
	orr	tmp2, data1, #REP8_7f
	eor	diff, data1, data2	/* Non-zero if differences found.  */
	bic	has_nul, tmp1, tmp2	/* Non-zero if NUL terminator.  */
	orr	syndrome, diff, has_nul
	cbz	syndrome, L(loop_aligned)
	/* End of performance-critical section  -- one 64B cache line.  */

L(end):
#ifdef __AARCH64EB__
	/* For big-endian we cannot use the trick with the syndrome value
	   as carry-propagation can corrupt the upper bits if the trailing
	   bytes in the string contain 0x01.  */
	/* However, if there is no NUL byte in the dword, we can generate
	   the result directly.  We can't just subtract the bytes as the
	   MSB might be significant.  */
	cbnz	has_nul, 1f
	cmp	data1, data2
	cset	result, ne
	cneg	result, result, lo
	ret
1:
	/* Re-compute the NUL-byte detection, using a byte-reversed value.  */
	rev	tmp3, data1
	sub	tmp1, tmp3, zeroones
	orr	tmp2, tmp3, #REP8_7f
	bic	has_nul, tmp1, tmp2
	rev	has_nul, has_nul
	orr	syndrome, diff, has_nul
	clz	pos, syndrome
	/* The most-significant-non-zero bit of the syndrome marks either the
	   first bit that is different, or the top bit of the first zero byte.
	   Shifting left now will bring the critical information into the
	   top bits.  */
	lsl	data1, data1, pos
	lsl	data2, data2, pos
	/* But we need to zero-extend (char is unsigned) the value and then
	   perform a signed 32-bit subtraction.  */
	lsr	data1, data1, #56
	sub	result, data1, data2, lsr #56
	ret
#endif

L(end_quick):
#ifndef __AARCH64EB__
	rev	syndrome, syndrome
	rev	data1, data1
#endif
	/* The most-significant-non-zero bit of the syndrome marks either the
	   first bit that is different, or the top bit of the first zero byte.
	   Shifting left now will bring the critical information into the
	   top bits.  */
	clz	pos, syndrome
#ifndef __AARCH64EB__
	rev	data2, data2
#endif
	lsl	data1, data1, pos
	lsl	data2, data2, pos
	/* But we need to zero-extend (char is unsigned) the value and then
	   perform a signed 32-bit subtraction.  */
	lsr	data1, data1, #56
	sub	result, data1, data2, lsr #56
	ret

L(mutual_align):
	/* Sources are mutually aligned, but are not currently at an
	   alignment boundary.  Round down the addresses and then mask off
	   the bytes that precede the start point.  */
	bic	src1, src1, #7
	bic	src2, src2, #7
	lsl	tmp1, tmp1, #3		/* Bytes beyond alignment -> bits.  */
	ldr	data1, [src1], #8
	neg	tmp1, tmp1		/* Bits to alignment -64.  */
	ldr	data2, [src2], #8
	mov	tmp2, #~0
	LS_FW	tmp2, tmp2, tmp1	/* Shift (tmp1 & 63).  */
	orr	data1, data1, tmp2
	orr	data2, data2, tmp2
	b	L(start_realigned)

	/* The following diagram explains the comparison of misaligned strings.
	   The bytes are shown in natural order. For little-endian, it is
	   reversed in the registers. The "x" bytes are before the string.
	   The "|" separates data that is loaded at one time.
	   src1     | a a a a a a a a | b b b c c c c c | . . .
	   src2     | x x x x x a a a   a a a a a b b b | c c c c c . . .

	   After shifting in each step, the data looks like this:
	                STEP_A              STEP_B              STEP_C
	   data1    a a a a a a a a     b b b c c c c c     b b b c c c c c
	   data2    a a a a a a a a     b b b 0 0 0 0 0     0 0 0 c c c c c

	   The bytes with "0" are eliminated from the syndrome via mask. */

L(misaligned8):
	/* Align SRC1 to 8 bytes and then compare 8 bytes at a time, always
	   checking to make sure that we don't access beyond page boundary in
	   SRC2.  */
	tst	src1, #7
	b.eq	L(src1_aligned)
L(do_misaligned):
	ldrb	data1w, [src1], #1
	ldrb	data2w, [src2], #1
	cmp	data1w, #1
	ccmp	data1w, data2w, #0, cs	/* NZCV = 0b0000.  */
	b.ne	L(done)
	tst	src1, #7
	b.ne	L(do_misaligned)

	/* Align SRC2 down to 16 bytes. This way we can read 16 bytes at a
	   time from SRC2. The comparison happens in 3 steps. After each step
	   the loop can exit, or read from SRC1 or SRC2. */
L(src1_aligned):
	/* Calculate offset from 8 byte alignment to string start in bits. No
	   need to mask offset since shifts are ignoring upper bits. */
	lsl	offset, src2, #3
	bic	src2, src2, #0xf
	mov	mask, -1
	neg	neg_offset, offset
	ldr	data1, [src1], #8
	ldp	tmp1, tmp2, [src2], #16
	LS_BK	mask, mask, neg_offset
	/* Skip the first compare if data in tmp1 is irrelevant. */
	tbnz	offset, 6, L(misaligned_mid_loop)

L(loop_misaligned):
	/* STEP_A: Compare full 8 bytes when there is enough data from SRC2.*/
	LS_FW	data2, tmp1, offset
	LS_BK	tmp1, tmp2, neg_offset
	sub	has_nul, data1, zeroones
	orr	data2, data2, tmp1	/* 8 bytes from SRC2 combined from two regs.*/
	orr	tmp3, data1, #REP8_7f
	eor	diff, data2, data1	/* Non-zero if differences found. */
	bic	has_nul, has_nul, tmp3	/* Non-zero if NUL byte found in SRC1. */
	orr	syndrome, diff, has_nul
	cbnz	syndrome, L(end)

	ldr	data1, [src1], #8
L(misaligned_mid_loop):
	/* STEP_B: Compare first part of data1 to second part of tmp2. */
	LS_FW	data2, tmp2, offset
#ifdef __AARCH64EB__
	/* For big-endian we do a byte reverse to avoid carry-propagation
	problem described above. This way we can reuse the has_nul in the
	next step and also use syndrome value trick at the end. */
	rev	tmp3, data1
	#define data1_fixed tmp3
#else
	#define data1_fixed data1
#endif
	sub	has_nul, data1_fixed, zeroones
	orr	tmp3, data1_fixed, #REP8_7f
	eor	diff, data2, data1	/* Non-zero if differences found.  */
	bic	has_nul, has_nul, tmp3	/* Non-zero if NUL terminator.  */
#ifdef __AARCH64EB__
	rev	has_nul, has_nul
#endif
	orr	syndrome, diff, has_nul
	bics	syndrome, syndrome, mask	/* Ignore later bytes. */
	b.ne	L(end_quick)

	/* STEP_C: Compare second part of data1 to first part of tmp1. */
	ldp	tmp1, tmp2, [src2], #16
	LS_BK	data2, tmp1, neg_offset
	eor	diff, data2, data1	/* Non-zero if differences found.  */
	orr	syndrome, diff, has_nul
	ands	syndrome, syndrome, mask	/* Ignore earlier bytes. */
	b.ne	L(end_quick)

	ldr	data1, [src1], #8
	b	L(loop_misaligned)

L(done):
	sub	result, data1, data2
	ret

END (__strcmp_aarch64_mte)

END_FILE
