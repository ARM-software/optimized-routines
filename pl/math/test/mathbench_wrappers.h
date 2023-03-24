/*
 * Function wrappers for mathbench.
 *
 * Copyright (c) 2022-2023, Arm Limited.
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

#if WANT_VMATH
#if __aarch64__ && defined(__vpcs)

__vpcs static v_double
_Z_atan2_wrap (v_double x)
{
  return _ZGVnN2vv_atan2 (v_double_dup (5.0), x);
}

__vpcs static v_float
_Z_atan2f_wrap (v_float x)
{
  return _ZGVnN4vv_atan2f (v_float_dup (5.0f), x);
}

__vpcs static v_double
xy_Z_pow (v_double x)
{
  return _ZGVnN2vv_pow (x, x);
}

__vpcs static v_double
x_Z_pow (v_double x)
{
  return _ZGVnN2vv_pow (x, v_double_dup (23.4));
}

__vpcs static v_double
y_Z_pow (v_double x)
{
  return _ZGVnN2vv_pow (v_double_dup (2.34), x);
}

#endif // __arch64__ && __vpcs
#endif // WANT_VMATH

#if WANT_SVE_MATH

static sv_float
_Z_sv_atan2f_wrap (sv_float x, sv_bool pg)
{
  return _ZGVsMxvv_atan2f (x, svdup_n_f32 (5.0f), pg);
}

static sv_double
_Z_sv_atan2_wrap (sv_double x, sv_bool pg)
{
  return _ZGVsMxvv_atan2 (x, svdup_n_f64 (5.0), pg);
}

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

static sv_double
xy_Z_sv_pow (sv_double x, sv_bool pg)
{
  return _ZGVsMxvv_pow (x, x, pg);
}

static sv_double
x_Z_sv_pow (sv_double x, sv_bool pg)
{
  return _ZGVsMxvv_pow (x, svdup_n_f64 (23.4), pg);
}

static sv_double
y_Z_sv_pow (sv_double x, sv_bool pg)
{
  return _ZGVsMxvv_pow (svdup_n_f64 (2.34), x, pg);
}

#endif // WANT_SVE_MATH
