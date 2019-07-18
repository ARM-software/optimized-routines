/*
 * Trivial AArch64 memcpy.
 *
 * Copyright (c) 2019, Arm Limited.
 * SPDX-License-Identifier: MIT
 */

#if __aarch64__
#include "asmdefs.h"

ENTRY (__memcpy_bytewise)
	cbz	x2, 2f
	mov	x3, 0
1:
	ldrb	w4, [x1, x3]
	strb	w4, [x0, x3]
	add	x3, x3, 1
	cmp	x3, x2
	bne	1b
2:
	ret
END (__memcpy_bytewise)
#endif
