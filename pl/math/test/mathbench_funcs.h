// clang-format off
/*
 * Function entries for mathbench.
 *
 * Copyright (c) 2022, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#ifdef __vpcs

#define ZVNF(f, a, b) F(__s_##f, a, b) VF(__v_##f, a, b) VNF(__vn_##f, a, b) VNF(_ZGVnN4v_##f, a, b)
#define ZVND(f, a, b) D(__s_##f, a, b) VD(__v_##f, a, b) VND(__vn_##f, a, b) VND(_ZGVnN2v_##f, a, b)

#elif __aarch64__

#define ZVNF(f, a, b) F(__s_##f, a, b) VF(__v_##f, a, b)
#define ZVND(f, a, b) D(__s_##f, a, b) VD(__v_##f, a, b)

#else

#define ZVNF(f, a, b) F(__s_##f, a, b)
#define ZVND(f, a, b) D(__s_##f, a, b)

#endif

#define VZSVF(f, a, b) SVF(__sv_##f##_x, a, b) SVF(_ZGVsMxv_##f, a, b)
#define VZSVD(f, a, b) SVD(__sv_##f##_x, a, b) SVD(_ZGVsMxv_##f, a, b)

F (acoshf, 1.0, 10.0)
F (asinhf, -10.0, 10.0)
F (atanf, -10.0, 10.0)
{"atan2f", 'f', 0, -10.0, 10.0, {.f = atan2f_wrap}},
F (cosf, -3.1, 3.1)
F (coshf, -10.0, 10.0)
F (erfcf, -4.0, 10.0)
F (erff, -4.0, 4.0)
F (expm1f, -9.9, 9.9)
F (log10f, 0.01, 11.1)
F (log1pf, -0.9, 10.0)
F (log2f, 0.01, 11.1)
F (sinf, -3.1, 3.1)
F (sinhf, -10.0, 10.0)
F (tanf, -3.1, 3.1)

D (acosh, 1.0, 10.0)
D (asinh, -10.0, 10.0)
D (atan, -10.0, 10.0)
{"atan2", 'd', 0, -10.0, 10.0, {.d = atan2_wrap}},
D (cos, -3.1, 3.1)
D (cosh, -10.0, 10.0)
D (erf, -6,6)
D (erfc, -6.0, 28.0)
D (expm1, -9.9, 9.9)
D (log10, 0.01, 11.1)
D (log1p, -0.9, 10.0)
D (log2, 0.01, 11.1)
{"powi", 'd', 0, 0.01, 11.1, {.d = powi_wrap}},
D (sin, -3.1, 3.1)
D (sinh, -10.0, 10.0)

#if WANT_VMATH
ZVNF (asinhf, -10.0, 10.0)
ZVNF (atanf, -10.0, 10.0)
ZVND (atan, -10.0, 10.0)
ZVNF (coshf, -10.0, 10.0)
ZVND (cosh, -10.0, 10.0)
ZVNF (erff, -4.0, 4.0)
ZVND (erf, -6.0, 6.0)
ZVNF (erfcf, -6.0, 28.0)
ZVND (erfc, -6.0, 28.0)
ZVNF (expm1f, -9.9, 9.9)
ZVND (expm1, -9.9, 9.9)
ZVNF (log10f, 0.01, 11.1)
ZVND (log10, 0.01, 11.1)
ZVNF (log1pf, -0.9, 10.0)
ZVND (log1p, -0.9, 10.0)
ZVNF (log2f, 0.01, 11.1)
ZVND (log2, 0.01, 11.1)
ZVNF (sinhf, -10.0, 10.0)
ZVND (sinh, -10.0, 10.0)
ZVNF (tanf, -3.1, 3.1)
{"__s_atan2f", 'f', 0, -10.0, 10.0, {.f = __s_atan2f_wrap}},
{"__s_atan2", 'd', 0, -10.0, 10.0, {.d = __s_atan2_wrap}},
{"__v_atan2f", 'f', 'v', -10.0, 10.0, {.vf = __v_atan2f_wrap}},
{"__v_atan2", 'd', 'v', -10.0, 10.0, {.vd = __v_atan2_wrap}},
{"__vn_atan2f", 'f', 'n', -10.0, 10.0, {.vnf = __vn_atan2f_wrap}},
{"_ZGVnN4vv_atan2f", 'f', 'n', -10.0, 10.0, {.vnf = _Z_atan2f_wrap}},
{"__vn_atan2", 'd', 'n', -10.0, 10.0, {.vnd = __vn_atan2_wrap}},
{"_ZGVnN2vv_atan2", 'd', 'n', -10.0, 10.0, {.vnd = _Z_atan2_wrap}},

#if WANT_SVE_MATH
VZSVF (atanf, -3.1, 3.1)
VZSVD (atan, -3.1, 3.1)
{"__sv_atan2f_x", 'f', 'n', -10.0, 10.0, {.svf = __sv_atan2f_wrap}},
{"_ZGVsMxvv_atan2f", 'f', 'n', -10.0, 10.0, {.svf = _Z_sv_atan2f_wrap}},
{"__sv_atan2_x", 'd', 'n', -10.0, 10.0, {.svd = __sv_atan2_wrap}},
{"_ZGVsM2vv_atan2", 'd', 'n', -10.0, 10.0, {.svd = _Z_sv_atan2_wrap}},
VZSVF (erff, -4.0, 4.0)
VZSVD (erf, -4.0, 4.0)
VZSVD (erfc, -4, 10)
VZSVF (expf, -9.9, 9.9)
VZSVF (cosf, -3.1, 3.1)
VZSVD (cos, -3.1, 3.1)
VZSVF (sinf, -3.1, 3.1)
VZSVD (sin, -3.1, 3.1)
VZSVF (logf, 0.01, 11.1)
VZSVD (log, 0.01, 11.1)
VZSVF (log10f, 0.01, 11.1)
VZSVD (log10, 0.01, 11.1)
VZSVF (tanf, -3.1, 3.1)
{"__sv_powif_x", 'f', 'n', -10.0, 10.0, {.svf = __sv_powif_wrap}},
{"_ZGVsMxvv_powi", 'f', 'n', -10.0, 10.0, {.svf = _Z_sv_powi_wrap}},
{"__sv_powi_x", 'd', 'n', -10.0, 10.0, {.svd = __sv_powi_wrap}},
{"_ZGVsMxvv_powk", 'd', 'n', -10.0, 10.0, {.svd = _Z_sv_powk_wrap}},

#endif
#endif
  // clang-format on
