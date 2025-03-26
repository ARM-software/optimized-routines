/*
 * Macros for asm code.  AArch64 version.
 *
 * Copyright (c) 2019-2025, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#ifndef _ASMDEFS_H
#define _ASMDEFS_H

/* Set the line separator for the assembler.  */
#if defined (__APPLE__)
# define SEP %%
# define PREF _
#else
# define SEP ;
#endif

/* Branch Target Identitication support.  */
#define BTI_C		hint	34
#define BTI_J		hint	36
/* Return address signing support (pac-ret).  */
#define PACIASP		hint	25 SEP .cfi_window_save
#define AUTIASP		hint	29 SEP .cfi_window_save

/* GNU_PROPERTY_AARCH64_* macros from elf.h.  */
#define FEATURE_1_AND 0xc0000000
#define FEATURE_1_BTI 1
#define FEATURE_1_PAC 2

/* Add a NT_GNU_PROPERTY_TYPE_0 note.  */
#define GNU_PROPERTY(type, value)	\
  .section .note.gnu.property, "a"  SEP \
  .p2align 3			    SEP \
  .word 4			    SEP \
  .word 16			    SEP \
  .word 5			    SEP \
  .asciz "GNU"			    SEP \
  .word type			    SEP \
  .word 4			    SEP \
  .word value			    SEP \
  .word 0			    SEP \
  .text

/* If set then the GNU Property Note section will be added to
   mark objects to support BTI and PAC-RET.  */
#ifndef WANT_GNU_PROPERTY
#define WANT_GNU_PROPERTY 1
#endif

#if WANT_GNU_PROPERTY
/* Add property note with supported features to all asm files.  */
GNU_PROPERTY (FEATURE_1_AND, FEATURE_1_BTI|FEATURE_1_PAC)
#endif

#define ENTRY_ALIGN(name, alignment)	\
  .align alignment		    SEP \
  ENTRY_ALIAS(name)		    SEP \
  .cfi_startproc		    SEP \
  BTI_C

#define ENTRY(name)	ENTRY_ALIGN(name, 6)

#if defined (__APPLE__)
/* Darwin is an underscore platform, symbols need an extra _ prefix.  */
# define ENTRY_ALIAS(name)	\
  .global _ ## name	    SEP \
  _ ## name:

# define END(name)	.cfi_endproc
#else
# define ENTRY_ALIAS(name)	\
  .global name		    SEP \
  name:

# define END(name)	.cfi_endproc
#else
# define ENTRY_ALIAS(name)	\
  .global name;			\
  .type name,%function;		\
  name:

# define END(name)	\
  .cfi_endproc;		\
  .size name, .-name
#endif

#define L(l) .L ## l

#endif
