/*
 * Macros for asm code.
 *
 * Copyright (c) 2019, Arm Limited.
 * SPDX-License-Identifier: MIT
 */

#ifndef _ASMDEFS_H
#define _ASMDEFS_H

#if defined(__aarch64__)

/* Branch Target Identitication support.  */
#define BTI_C		hint	34
#define BTI_J		hint	36
/* Return address signing support (pac-ret).  */
#define PACIASP		hint	25; .cfi_window_save
#define AUTIASP		hint	29; .cfi_window_save

#define FEATURE_1_BTI 1
#define FEATURE_1_PAC 2

/* Add a GNU_PROPERTY_AARCH64_FEATURE_1_AND note.  */
#define GNU_PROPERTY(features)		\
  .section .note.gnu.property, "a";	\
  .p2align 3;				\
  .word 4;				\
  .word 16;				\
  .word 5;				\
  .asciz "GNU";				\
  .word 0xc0000000;			\
  .word 4;				\
  .word features;			\
  .word 0;

#define END_FILE GNU_PROPERTY(FEATURE_1_BTI|FEATURE_1_PAC)

#define ENTRY_ALIGN(name, alignment)	\
  .global name;		\
  .type name,%function;	\
  .align alignment;		\
  name:			\
  .cfi_startproc;	\
  BTI_C;

#else

#define END_FILE

#define ENTRY_ALIGN(name, alignment)	\
  .global name;		\
  .type name,%function;	\
  .align alignment;		\
  name:			\
  .cfi_startproc;

#endif

#define ENTRY(name)	ENTRY_ALIGN(name, 6)

#define ENTRY_ALIAS(name)	\
  .global name;		\
  .type name,%function;	\
  name:

#define END(name)	\
  .cfi_endproc;		\
  .size name, .-name;

#define L(l) .L ## l

#endif
