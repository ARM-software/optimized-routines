/*
 * Public API.
 *
 * Copyright (c) 2019, Arm Limited.
 * SPDX-License-Identifier: MIT
 */

#include <stddef.h>

/* restrict is not needed, but kept for documenting the interface contract.  */
#ifndef __restrict
# define __restrict
#endif

#if __aarch64__
void *__memcpy_bytewise (void *__restrict, const void *__restrict, size_t);
#endif
