/*
 * Function wrappers for mathbench.
 *
 * Copyright (c) 2022, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

static double
atan2_wrap (double x)
{
  return atan2 (5.0, x);
}

static float
atan2f_wrap (float x)
{
  return atan2f (5.0f, x);
}

#if WANT_VMATH
#if __aarch64__

static double
__s_atan2_wrap (double x)
{
  return __s_atan2 (5.0, x);
}

static v_double
__v_atan2_wrap (v_double x)
{
  return __v_atan2 (v_double_dup (5.0), x);
}

#ifdef __vpcs

__vpcs static v_double
__vn_atan2_wrap (v_double x)
{
  return __vn_atan2 (v_double_dup (5.0), x);
}

__vpcs static v_double
_Z_atan2_wrap (v_double x)
{
  return _ZGVnN2vv_atan2 (v_double_dup (5.0), x);
}

#endif // __vpcs
#endif // __arch64__
#endif // WANT_VMATH
