/*
 * Macros for pacbti asm code.
 *
 * Copyright (c) 2022, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

/* Macro to handle function entry depending on branch-protection
   schemes */
	.macro pacbti_prologue
#if __ARM_FEATURE_PAC_DEFAULT
#if __ARM_FEATURE_BTI_DEFAULT
	pacbti ip, lr, sp
#else
	pac ip, lr, sp
#endif /* __ARM_FEATURE_BTI_DEFAULT */
	str ip, [sp, #-4]!
	.save {ra_auth_code}
	.cfi_def_cfa_offset 4
	.cfi_offset 143, -4
#elif __ARM_FEATURE_BTI_DEFAULT
	bti
#endif /* __ARM_FEATURE_PAC_DEFAULT */
	.endm

/* Macro to handle different branch exchange cases depending on
   branch-protection schemes */
	.macro pacbti_epilogue
#if __ARM_FEATURE_PAC_DEFAULT
	ldr ip, [sp], #4
	.cfi_restore 143
	.cfi_def_cfa_offset 0
	aut ip, lr, sp
#endif /* __ARM_FEATURE_PAC_DEFAULT */
	bx lr
	.endm
