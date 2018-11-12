/*
 * dunder.c - manually provoke FP exceptions for mathlib
 *
 * Copyright (c) 2009-2018, Arm Limited.
 * SPDX-License-Identifier: MIT
 */


#include "math_private.h"
#include <fenv.h>

__inline double __mathlib_dbl_infnan(double x)
{
  return x+x;
}

__inline double __mathlib_dbl_infnan2(double x, double y)
{
  return x+y;
}

double __mathlib_dbl_underflow(void)
{
#ifdef CLANG_EXCEPTIONS
  feraiseexcept(FE_UNDERFLOW);
#endif
  return 0x1p-767 * 0x1p-767;
}

double __mathlib_dbl_overflow(void)
{
#ifdef CLANG_EXCEPTIONS
  feraiseexcept(FE_OVERFLOW);
#endif
  return 0x1p+769 * 0x1p+769;
}

double __mathlib_dbl_invalid(void)
{
#ifdef CLANG_EXCEPTIONS
  feraiseexcept(FE_INVALID);
#endif
  return 0.0 / 0.0;
}

double __mathlib_dbl_divzero(void)
{
#ifdef CLANG_EXCEPTIONS
  feraiseexcept(FE_DIVBYZERO);
#endif
  return 1.0 / 0.0;
}
