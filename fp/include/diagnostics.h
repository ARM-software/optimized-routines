// Diagnostic macros for printing out registers in assembly language functions
//
// Copyright (c) 2025, Arm Limited.
// SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception

#ifndef AOR_FP_DIAGNOSTICS_H
#define AOR_FP_DIAGNOSTICS_H

// This header file defines two debugging macros, 'debug32' and 'debug64'.
//
// Normally, they expand to no code at all. But if you compile with
// -DDIAGNOSTICS, they expand to code which prints the value of one register or
// two registers respectively, by calling printf with a provided format string.
// So a function can be annotated with calls to these macros without affecting
// its performance at all, and then it's easy to recompile that function so
// that it prints out useful intermediate values in its computations.
//
// We assume that performance isn't important in diagnostic mode (or else you'd
// be calling something much less heavyweight than full printf). So these
// macros call printf in the simplest possible way, without concern for
// efficient stack usage. In particular, debug64 transfers the two input
// registers into the right places by storing them into temporary stack slots
// and then reloading, which is slow, but the easiest way to avoid ordering
// problems if the input registers alias r2 and r3.

#include "endian.h" // for register aliases bh and bl

#ifdef DIAGNOSTICS

#if __thumb__ && __ARM_ARCH_ISA_THUMB == 1
#error These diagnostic macros do not currently support Thumb-1
#endif

  // debug32 passes a single 32-bit register to printf, so you should provide a
  // format string which expects a 32-bit integer, e.g. based on %x.
  //
  // Usage: debug32 "format string", register
  // e.g.   debug32 "r0 = %08x", r0
  .macro  debug32 text,reg
  push    {r0,r1,r2,r3,r12,lr} // save all the registers we're going to clobber
  mov     r1,\reg              // copy the reg to be printed into r1
  adr     r0, 1f               // address of the format string in r0
  bl      printf               // call out to printf
  pop     {r0,r1,r2,r3,r12,lr} // and pop all the registers again
  B       0f                   // jump over the format string
1: .asciz "\text"
  .p2align 2
0:
  .endm

  // debug64 passes two 32-bit registers to printf, in r2 and r3, where printf
  // will read a 64-bit integer parameter from. So you should provide a format
  // string which expects a 64-bit integer, e.g. based on %llx. The register
  // parameters to the macro should give the high-order register first.
  //
  // Usage: debug64 "format string", highreg, lowreg
  // e.g.   debug64 "(r5 << 32) + r6 = %016llx", r5, r6
  .macro  debug64 text,regh,regl
  push    {r0,r1,r2,r3,r12,lr} // save all the registers we're going to clobber
  sub     sp,sp,#8             // make some space on the stack
  str     \regh,[sp,#0]        // store the two input registers to that space
  str     \regl,[sp,#4]
  ldr     bh,[sp,#0]           // reload into bh,bl to pass to printf
  ldr     bl,[sp,#4]
  add     sp,sp,#8             // we've finished with those 2 words of stack
  adr     r0, 1f               // address of the format string in r0
  bl      printf               // call out to printf
  pop     {r0,r1,r2,r3,r12,lr} // and pop all the registers again
  B       0f                   // jump over the format string
1: .asciz "\text"
  .p2align 2
0:
  .endm

#else // DIAGNOSTICS

  // In non-DIAGNOSTICS mode, debug32 and debug64 are empty macros.
  .macro debug32 text,reg
  .endm
  .macro debug64 text,regh,regl
  .endm

#endif // DIAGNOSTICS

#endif // AOR_FP_DIAGNOSTICS_H
