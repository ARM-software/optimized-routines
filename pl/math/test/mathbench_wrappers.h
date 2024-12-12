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

#if WANT_SIMD_TESTS

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

#endif // __arch64__ && __vpcs

#if WANT_SVE_MATH

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
_Z_sv_powi_wrap (sv_float x, sv_bool pg)
{
  return _ZGVsMxvv_powi (x, svcvt_s32_f32_x (pg, x), pg);
}

static sv_double
_Z_sv_powk_wrap (sv_double x, sv_bool pg)
{
  return _ZGVsMxvv_powk (x, svcvt_s64_f64_x (pg, x), pg);
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

#endif // WANT_SVE_MATH
