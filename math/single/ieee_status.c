/*
 * ieee_status.c
 *
 * Copyright (c) 2015-2018, Arm Limited.
 * SPDX-License-Identifier: MIT
 */

#include "math_private.h"

__inline unsigned __ieee_status(unsigned bicmask, unsigned xormask)
{
#if defined __aarch64__ && defined __FP_FENV_EXCEPTIONS
  unsigned status_word;
  unsigned ret;

#ifdef __FP_FENV_ROUNDING
# define MASK (1<<27)|FE_IEEE_FLUSHZERO|FE_IEEE_MASK_ALL_EXCEPT|FE_IEEE_ALL_EXCEPT|FE_IEEE_ROUND_MASK
#else
# define MASK (1<<27)|FE_IEEE_FLUSHZERO|FE_IEEE_MASK_ALL_EXCEPT|FE_IEEE_ALL_EXCEPT
#endif

  /* mask out read-only bits */
  bicmask &= MASK;
  xormask &= MASK;

  /* modify the status word */
  __asm__ __volatile__ ("mrs %0, fpsr" : "=r" (status_word));
  ret = status_word;
  status_word &= ~bicmask;
  status_word ^= xormask;
  __asm__ __volatile__ ("msr fpsr, %0" : : "r" (status_word));

  /* and return what it used to be */
  return ret;
#else
  return 0;
#endif
}
