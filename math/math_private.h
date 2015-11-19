/*
 *  math_private.h
 *
 *  Copyright (C) 2015, ARM Limited, All Rights Reserved
 *  SPDX-License-Identifier: Apache-2.0
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may
 *  not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  This file is part of the Optimized Routines project
 */

/*
 * Header file containing some definitions and other small reusable pieces of
 * code that we need in the libraries.
 */

#ifndef __MATH_PRIVATE_H
#define __MATH_PRIVATE_H

#include <errno.h>
#include <stdint.h>

extern int    ARM__ieee754_rem_pio2(double, double *);
extern double ARM__kernel_sin(double, double, int);
extern double ARM__kernel_cos(double, double);
extern double ARM__kernel_poly(const double *, int, double);

#define __FP_IEEE
#define __FP_FENV_EXCEPTIONS
#define __FP_FENV_ROUNDING
#define __FP_INEXACT_EXCEPTION

#define __set_errno(val) (errno = (val))

#define __FLT(x) (*(unsigned *)&(x))
#if defined(__ARM_BIG_ENDIAN) || defined(__BIG_ENDIAN)
#  define __LO(x) (*(1 + (unsigned *)&(x)))
#  define __HI(x) (*(unsigned *)&(x))
#else /* !defined(__ARM_BIG_ENDIAN) && !defined(__BIG_ENDIAN) */
#  define __HI(x) (*(1 + (unsigned *)&(x)))
#  define __LO(x) (*(unsigned *)&(x))
#endif /* !defined(__ARM_BIG_ENDIAN) && !defined(__BIG_ENDIAN) */

// FIXME: Implement these without type punning.
static __inline unsigned int fai(float f) { return __FLT(f); }
static __inline float fhex(unsigned int n) { float f; __FLT(f) = n; return f; }

#define CLEARBOTTOMHALF(x) fhex((fai(x) + 0x00000800) & 0xFFFFF000)

#define FE_IEEE_OVERFLOW           (0x00000004)
#define FE_IEEE_UNDERFLOW          (0x00000008)
#define FE_IEEE_FLUSHZERO          (0x01000000)
#define FE_IEEE_ROUND_TONEAREST    (0x00000000)
#define FE_IEEE_ROUND_UPWARD       (0x00400000)
#define FE_IEEE_ROUND_DOWNWARD     (0x00800000)
#define FE_IEEE_ROUND_TOWARDZERO   (0x00C00000)
#define FE_IEEE_ROUND_MASK         (0x00C00000)
#define FE_IEEE_MASK_INVALID       (0x00000100)
#define FE_IEEE_MASK_DIVBYZERO     (0x00000200)
#define FE_IEEE_MASK_OVERFLOW      (0x00000400)
#define FE_IEEE_MASK_UNDERFLOW     (0x00000800)
#define FE_IEEE_MASK_INEXACT       (0x00001000)
#define FE_IEEE_MASK_INPUTDENORMAL (0x00008000)
#define FE_IEEE_MASK_ALL_EXCEPT    (0x00009F00)
#define FE_IEEE_INVALID            (0x00000001)
#define FE_IEEE_DIVBYZERO          (0x00000002)
#define FE_IEEE_INEXACT            (0x00000010)
#define FE_IEEE_INPUTDENORMAL      (0x00000080)
#define FE_IEEE_ALL_EXCEPT         (0x0000009F)

extern double __mathlib_dbl_overflow(void);
extern float __mathlib_flt_overflow(void);
extern double __mathlib_dbl_underflow(void);
extern float __mathlib_flt_underflow(void);
extern double __mathlib_dbl_invalid(void);
extern float __mathlib_flt_invalid(void);
extern double __mathlib_dbl_divzero(void);
extern float __mathlib_flt_divzero(void);
#define DOUBLE_OVERFLOW ( __mathlib_dbl_overflow() )
#define FLOAT_OVERFLOW ( __mathlib_flt_overflow() )
#define DOUBLE_UNDERFLOW ( __mathlib_dbl_underflow() )
#define FLOAT_UNDERFLOW ( __mathlib_flt_underflow() )
#define DOUBLE_INVALID ( __mathlib_dbl_invalid() )
#define FLOAT_INVALID  ( __mathlib_flt_invalid() )
#define DOUBLE_DIVZERO ( __mathlib_dbl_divzero() )
#define FLOAT_DIVZERO  ( __mathlib_flt_divzero() )

extern float  __mathlib_flt_infnan(float);
extern float  __mathlib_flt_infnan2(float, float);
extern double __mathlib_dbl_infnan(double);
extern double __mathlib_dbl_infnan2(double, double);
extern unsigned __ieee_status(unsigned, unsigned);
extern double ARM__kernel_tan(double, double, int);

#define FLOAT_INFNAN(x) __mathlib_flt_infnan(x)
#define FLOAT_INFNAN2(x,y) __mathlib_flt_infnan2(x,y)
#define DOUBLE_INFNAN(x) __mathlib_dbl_infnan(x)
#define DOUBLE_INFNAN2(x,y) __mathlib_dbl_infnan2(x,y)

#define MATHERR_POWF_00(x,y) (__set_errno(EDOM), 1.0f)
#define MATHERR_POWF_INF0(x,y) (__set_errno(EDOM), 1.0f)
#define MATHERR_POWF_0NEG(x,y) (__set_errno(ERANGE), FLOAT_DIVZERO)
#define MATHERR_POWF_NEG0FRAC(x,y) (0.0f)
#define MATHERR_POWF_0NEGODD(x,y) (__set_errno(ERANGE), -FLOAT_DIVZERO)
#define MATHERR_POWF_0NEGEVEN(x,y) (__set_errno(ERANGE), FLOAT_DIVZERO)
#define MATHERR_POWF_NEGFRAC(x,y) (__set_errno(EDOM), FLOAT_INVALID)
#define MATHERR_POWF_ONEINF(x,y) (1.0f)
#define MATHERR_POWF_OFL(x,y,z) (__set_errno(ERANGE), copysignf(FLOAT_OVERFLOW,z))
#define MATHERR_POWF_UFL(x,y,z) (__set_errno(ERANGE), copysignf(FLOAT_UNDERFLOW,z))

#define MATHERR_LOGF_0(x) (__set_errno(ERANGE), -FLOAT_DIVZERO)
#define MATHERR_LOGF_NEG(x) (__set_errno(EDOM), FLOAT_INVALID)

#define MATHERR_SIN_INF(x) (__set_errno(EDOM), DOUBLE_INVALID)
#define MATHERR_SINF_INF(x) (__set_errno(EDOM), FLOAT_INVALID)
#define MATHERR_COS_INF(x) (__set_errno(EDOM), DOUBLE_INVALID)
#define MATHERR_COSF_INF(x) (__set_errno(EDOM), FLOAT_INVALID)
#define MATHERR_TAN_INF(x) (__set_errno(EDOM), DOUBLE_INVALID)
#define MATHERR_TANF_INF(x) (__set_errno(EDOM), FLOAT_INVALID)

#define MATHERR_EXPF_UFL(x) (__set_errno(ERANGE), FLOAT_UNDERFLOW)
#define MATHERR_EXPF_OFL(x) (__set_errno(ERANGE), FLOAT_OVERFLOW)

#define FLOAT_CHECKDENORM(x) ( (fpclassify(x) == FP_SUBNORMAL ? FLOAT_UNDERFLOW : 0), x )
#define DOUBLE_CHECKDENORM(x) ( (fpclassify(x) == FP_SUBNORMAL ? DOUBLE_UNDERFLOW : 0), x )

#endif /* __MATH_PRIVATE_H */
