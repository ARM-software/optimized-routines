/*
 * Function wrappers for mathbench.
 *
 * Copyright (c) 2022-2024, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#if WANT_SIMD_TESTS

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

__vpcs static v_float
_Z_hypotf_wrap (v_float x)
{
  return _ZGVnN4vv_hypotf (v_float_dup (5.0f), x);
}

__vpcs static v_double
_Z_hypot_wrap (v_double x)
{
  return _ZGVnN2vv_hypot (v_double_dup (5.0), x);
}

__vpcs static v_float
xy_Z_powf (v_float x)
{
  return _ZGVnN4vv_powf (x, x);
}

__vpcs static v_float
x_Z_powf (v_float x)
{
  return _ZGVnN4vv_powf (x, v_float_dup (23.4));
}

__vpcs static v_float
y_Z_powf (v_float x)
{
  return _ZGVnN4vv_powf (v_float_dup (2.34), x);
}

__vpcs static v_double
xy_Z_pow (v_double x)
{
  return _ZGVnN2vv_pow (x, x);
}

#endif

#if WANT_SVE_MATH

static sv_float
_Z_sv_atan2f_wrap (sv_float x, sv_bool pg)
{
  return _ZGVsMxvv_atan2f (x, svdup_f32 (5.0f), pg);
}

static sv_double
_Z_sv_atan2_wrap (sv_double x, sv_bool pg)
{
  return _ZGVsMxvv_atan2 (x, svdup_f64 (5.0), pg);
}

static sv_float
_Z_sv_hypotf_wrap (sv_float x, sv_bool pg)
{
  return _ZGVsMxvv_hypotf (x, svdup_f32 (5.0), pg);
}

static sv_double
_Z_sv_hypot_wrap (sv_double x, sv_bool pg)
{
  return _ZGVsMxvv_hypot (x, svdup_f64 (5.0), pg);
}

#endif

static double
xypow (double x)
{
  return pow (x, x);
}

static float
xypowf (float x)
{
  return powf (x, x);
}

static double
xpow (double x)
{
  return pow (x, 23.4);
}

static float
xpowf (float x)
{
  return powf (x, 23.4f);
}

static double
ypow (double x)
{
  return pow (2.34, x);
}

static float
ypowf (float x)
{
  return powf (2.34f, x);
}

static float
sincosf_wrap (float x)
{
  float s, c;
  sincosf (x, &s, &c);
  return s + c;
}
