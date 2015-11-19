/*
 *  dunder.c - manually provoke FP exceptions for mathlib
 *
 *  Copyright (C) 2009-2015, ARM Limited, All Rights Reserved
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
