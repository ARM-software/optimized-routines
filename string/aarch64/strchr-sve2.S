/*
 * strchr/strchrnul - find a character in a string
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

// To build as strchrnul, define BUILD_STRCHRNUL before compiling this file.
#if defined (BUILD_STRCHRNUL)
#define FUNC  __strchrnul_aarch64_sve2
#else
#define FUNC  __strchr_aarch64_sve2
#endif

#define srcin		x0		// const char* src.
#define chrin		w1		// int chr.
#define result		x0		// Return.

#define	vlalign		x2		// Alignment mask.
#define	src		x3		// Current data pointer.
#define	srcincopy	x4		// Copy of src pointer.

#define zdata		z0		// Data.
#define zchr_nul	z1		// chr and null data for search.
#define zchr		z2		// chr data for search.

#define pall		p0		// All active predicate.
#define pchr_nul	p1		// chr and null search result.
#define pchr		p2		// chr search result.
#define p1stchunk	p3		// Mask for first chunk.
#define pfirst		p4		// First found.

ENTRY (FUNC)
	dup	zchr.b, chrin		// Initialize chr data for search.
	and	chrin, chrin, #0xff	// Set zeros after first byte.
	dup	zchr_nul.h, chrin	// Initialize chr and null for search.

	mov	vlalign, #-1		// Create alignment mask based on VL.
	incb	vlalign

	mov	srcincopy, srcin	// Copy srcin pointer
	bic	src, srcin, vlalign	// Align src to VL.
	ptrue	pall.b			// Initialize all active.

	.p2align 4
L(loop):
	ld1b	{zdata.b}, pall/z, [src]	// Read	next chunk.
	match	pchr_nul.b, pall/z, zdata.b, zchr_nul.b	// Find chr or null.
	b.any	L(loop_end)	// If chr or null are found, move to end.
L(loop2):
	ld1b	{zdata.b}, pall/z, [src, #1, mul vl]	// Read	next chunk.
	incb	src, all, mul #2	// Increment by 2xVL.
	match	pchr_nul.b, pall/z, zdata.b, zchr_nul.b	// Find chr or null.
	b.none	L(loop)		// If chr or null are not found, repeat loop.

	decb	src			// Decrement by VL.
L(loop_end):
	mov	result, src		// Copy current pointer to result.
	brkb	pfirst.b, pall/z, pchr_nul.b	// Find first element.
	incp	result, pfirst.b	// Increment result by element index.
	cmp	result, srcincopy	// Is result before srcin?
	b.lo	L(first_chunk)	// If yes, first chunk should be handled.
#if !defined (BUILD_STRCHRNUL)
	match	pchr.b, pchr_nul/z, zdata.b, zchr.b	// Find only chr.
	csel	result, result, xzr, first	// If it is not, return null.
#endif
	ret

	.p2align 4
L(first_chunk):
	whilelo	p1stchunk.b, src, srcincopy	// Mask for first chunk.
	// Apply first chunk mask.
	bics	pchr_nul.b, pall/z, pchr_nul.b, p1stchunk.b
	b.any	L(loop_end)	// If chr or null are found, move to end.
	b	L(loop2)	// If not, proceed search in loop

END (FUNC)
