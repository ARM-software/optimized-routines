/*
 * Macros for asm code.
 *
 * Copyright (c) 2019, Arm Limited.
 * SPDX-License-Identifier: MIT
 */

#define ENTRY(name)	\
  .global name;		\
  .type name,%function;	\
  .align 4;		\
  name:			\
  .cfi_startproc;

#define END(name)	\
  .cfi_endproc;		\
  .size name, .-name;
