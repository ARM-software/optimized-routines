/*
 * ieee_status.c
 *
 * Copyright (c) 2015, Arm Limited.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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
