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
void *__memcpy_aarch64 (void *__restrict, const void *__restrict, size_t);
void *__memmove_aarch64 (void *__restrict, const void *__restrict, size_t);
void *__memset_aarch64 (void *, int, size_t);
void *__memchr_aarch64 (const void *, int, size_t);
#endif
