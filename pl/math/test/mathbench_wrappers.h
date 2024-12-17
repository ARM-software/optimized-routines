/*
 * Function wrappers for mathbench.
 *
 * Copyright (c) 2022-2024, Arm Limited.
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

static double
powi_wrap (double x)
{
  return __builtin_powi (x, (int) round (x));
}

#if WANT_SVE_MATH

static sv_float
_Z_sv_powi_wrap (sv_float x, sv_bool pg)
{
  return _ZGVsMxvv_powi (x, svcvt_s32_f32_x (pg, x), pg);
}

static sv_double
_Z_sv_powk_wrap (sv_double x, sv_bool pg)
{
  return _ZGVsMxvv_powk (x, svcvt_s64_f64_x (pg, x), pg);
}

#endif // WANT_SVE_MATH
