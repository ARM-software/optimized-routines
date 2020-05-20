/*
 * Public API.
 *
 * Copyright (c) 2019-2020, Arm Limited.
 * SPDX-License-Identifier: MIT
 */

#include <stddef.h>

/* restrict is not needed, but kept for documenting the interface contract.  */
#ifndef __restrict
# define __restrict
#endif

#if __aarch64__
void *__memcpy_aarch64 (void *__restrict, const void *__restrict, size_t);
void *__memmove_aarch64 (void *, const void *, size_t);
void *__memset_aarch64 (void *, int, size_t);
void *__memchr_aarch64 (const void *, int, size_t);
void *__memrchr_aarch64 (const void *, int, size_t);
int __memcmp_aarch64 (const void *, const void *, size_t);
char *__strcpy_aarch64 (char *__restrict, const char *__restrict);
char *__stpcpy_aarch64 (char *__restrict, const char *__restrict);
int __strcmp_aarch64 (const char *, const char *);
char *__strchr_aarch64 (const char *, int);
char *__strrchr_aarch64 (const char *, int);
char *__strchrnul_aarch64 (const char *, int );
size_t __strlen_aarch64 (const char *);
size_t __strnlen_aarch64 (const char *, size_t);
int __strncmp_aarch64 (const char *, const char *, size_t);
void * __memchr_aarch64_mte (const void *, int, size_t);
char *__strcpy_aarch64_mte (char *__restrict, const char *__restrict);
char *__stpcpy_aarch64_mte (char *__restrict, const char *__restrict);
char *__strchr_aarch64_mte (const char *, int);
char * __strchrnul_aarch64_mte (const char *, int );
size_t __strlen_aarch64_mte (const char *);
char *__strrchr_aarch64_mte (const char *, int);
int __strcmp_aarch64_mte (const char *, const char *);
int __strncmp_aarch64_mte (const char *, const char *, size_t);
#if __ARM_NEON
void *__memcpy_aarch64_simd (void *__restrict, const void *__restrict, size_t);
void *__memmove_aarch64_simd (void *, const void *, size_t);
#endif
# if __ARM_FEATURE_SVE
void *__memchr_aarch64_sve (const void *, int, size_t);
int __memcmp_aarch64_sve (const void *, const void *, size_t);
char *__strchr_aarch64_sve (const char *, int);
char *__strrchr_aarch64_sve (const char *, int);
char *__strchrnul_aarch64_sve (const char *, int );
int __strcmp_aarch64_sve (const char *, const char *);
char *__strcpy_aarch64_sve (char *__restrict, const char *__restrict);
char *__stpcpy_aarch64_sve (char *__restrict, const char *__restrict);
size_t __strlen_aarch64_sve (const char *);
size_t __strnlen_aarch64_sve (const char *, size_t);
int __strncmp_aarch64_sve (const char *, const char *, size_t);
# endif
#elif __arm__
void *__memcpy_arm (void *__restrict, const void *__restrict, size_t);
void *__memset_arm (void *, int, size_t);
void *__memchr_arm (const void *, int, size_t);
char *__strcpy_arm (char *__restrict, const char *__restrict);
int __strcmp_arm (const char *, const char *);
int __strcmp_armv6m (const char *, const char *);
size_t __strlen_armv6t2 (const char *);
#endif
