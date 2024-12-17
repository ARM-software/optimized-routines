/*
 * Function wrappers for mathbench.
 *
 * Copyright (c) 2022-2024, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#if WANT_SIMD_TESTS

__vpcs static v_float
_Z_sincospif_wrap (v_float x)
{
  v_float s, c;
  _ZGVnN4vl4l4_sincospif (x, &s, &c);
  return s + c;
}

__vpcs static v_double
_Z_sincospi_wrap (v_double x)
{
  v_double s, c;
  _ZGVnN2vl8l8_sincospi (x, &s, &c);
  return s + c;
}

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

__vpcs static v_float
_Z_modff_wrap (v_float x)
{
  v_float y;
  v_float ret = _ZGVnN4vl4_modff (x, &y);
  return ret + y;
}

__vpcs static v_double
_Z_modf_wrap (v_double x)
{
  v_double y;
  v_double ret = _ZGVnN2vl8_modf (x, &y);
  return ret + y;
}

__vpcs static v_float
_Z_sincosf_wrap (v_float x)
{
  v_float s, c;
  _ZGVnN4vl4l4_sincosf (x, &s, &c);
  return s + c;
}

__vpcs static v_float
_Z_cexpif_wrap (v_float x)
{
  __f32x4x2_t sc = _ZGVnN4v_cexpif (x);
  return sc.val[0] + sc.val[1];
}

__vpcs static v_double
_Z_sincos_wrap (v_double x)
{
  v_double s, c;
  _ZGVnN2vl8l8_sincos (x, &s, &c);
  return s + c;
}

__vpcs static v_double
_Z_cexpi_wrap (v_double x)
{
  __f64x2x2_t sc = _ZGVnN2v_cexpi (x);
  return sc.val[0] + sc.val[1];
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

static sv_float
xy_Z_sv_powf (sv_float x, sv_bool pg)
{
  return _ZGVsMxvv_powf (x, x, pg);
}

static sv_float
x_Z_sv_powf (sv_float x, sv_bool pg)
{
  return _ZGVsMxvv_powf (x, svdup_f32 (23.4f), pg);
}

static sv_float
y_Z_sv_powf (sv_float x, sv_bool pg)
{
  return _ZGVsMxvv_powf (svdup_f32 (2.34f), x, pg);
}

static sv_double
xy_Z_sv_pow (sv_double x, sv_bool pg)
{
  return _ZGVsMxvv_pow (x, x, pg);
}

static sv_double
x_Z_sv_pow (sv_double x, sv_bool pg)
{
  return _ZGVsMxvv_pow (x, svdup_f64 (23.4), pg);
}

static sv_double
y_Z_sv_pow (sv_double x, sv_bool pg)
{
  return _ZGVsMxvv_pow (svdup_f64 (2.34), x, pg);
}

static sv_float
_Z_sv_sincospif_wrap (sv_float x, sv_bool pg)
{
  float s[svcntw ()], c[svcntw ()];
  _ZGVsMxvl4l4_sincospif (x, s, c, pg);
  return svadd_x (pg, svld1 (pg, s), svld1 (pg, c));
}

static sv_double
_Z_sv_sincospi_wrap (sv_double x, sv_bool pg)
{
  double s[svcntd ()], c[svcntd ()];
  _ZGVsMxvl8l8_sincospi (x, s, c, pg);
  return svadd_x (pg, svld1 (pg, s), svld1 (pg, c));
}

static sv_float
_Z_sv_modff_wrap (sv_float x, sv_bool pg)
{
  float i[svcntw ()];
  svfloat32_t r = _ZGVsMxvl4_modff (x, i, pg);
  return svadd_x (pg, r, svld1 (pg, i));
}

static sv_double
_Z_sv_modf_wrap (sv_double x, sv_bool pg)
{
  double i[svcntd ()];
  svfloat64_t r = _ZGVsMxvl8_modf (x, i, pg);
  return svadd_x (pg, r, svld1 (pg, i));
}

static sv_float
_Z_sv_sincosf_wrap (sv_float x, sv_bool pg)
{
  float s[svcntw ()], c[svcntw ()];
  _ZGVsMxvl4l4_sincosf (x, s, c, pg);
  return svadd_x (pg, svld1 (pg, s), svld1 (pg, s));
}

static sv_float
_Z_sv_cexpif_wrap (sv_float x, sv_bool pg)
{
  svfloat32x2_t sc = _ZGVsMxv_cexpif (x, pg);
  return svadd_x (pg, svget2 (sc, 0), svget2 (sc, 1));
}

static sv_double
_Z_sv_sincos_wrap (sv_double x, sv_bool pg)
{
  double s[svcntd ()], c[svcntd ()];
  _ZGVsMxvl8l8_sincos (x, s, c, pg);
  return svadd_x (pg, svld1 (pg, s), svld1 (pg, s));
}

static sv_double
_Z_sv_cexpi_wrap (sv_double x, sv_bool pg)
{
  svfloat64x2_t sc = _ZGVsMxv_cexpi (x, pg);
  return svadd_x (pg, svget2 (sc, 0), svget2 (sc, 1));
}

# if WANT_EXPERIMENTAL_MATH

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

# endif

#endif

#if __aarch64__
static float
sincospif_wrap (float x)
{
  float s, c;
  arm_math_sincospif (x, &s, &c);
  return s + c;
}

static double
sincospi_wrap (double x)
{
  double s, c;
  arm_math_sincospi (x, &s, &c);
  return s + c;
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
