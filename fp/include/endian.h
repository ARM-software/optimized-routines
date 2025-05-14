// Register aliases for endian-independent floating point code.
//
// Copyright (c) 2025, Arm Limited.
// SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception

// This header file should be included from assembly source code (not C). It
// defines two pairs of register aliases, for handling 64-bit values passed and
// returned from functions in the AArch32 integer registers:
//
//   ah, al      the high and low words of a 64-bit value passed in {r0,r1}
//   bh, bl      the high and low words of a 64-bit value passed in {r2,r3}
//
// Which alias goes with which register depends on endianness.

#ifdef __BIG_ENDIAN__
// Big-endian: high words are in lower-numbered registers.
ah .req r0
al .req r1
bh .req r2
bl .req r3
#else
// Little-endian: low words are in lower-numbered registers.
al .req r0
ah .req r1
bl .req r2
bh .req r3
#endif

