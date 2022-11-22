/*
 * Function entries for ulp.
 *
 * Copyright (c) 2022, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#ifdef __vpcs

#define _ZVNF1(f) SF1 (f) VF1 (f) ZVNF1 (f)
#define _ZVND1(f) SD1 (f) VD1 (f) ZVND1 (f)
#define _ZVNF2(f) SF2 (f) VF2 (f) ZVNF2 (f)
#define _ZVND2(f) SD2 (f) VD2 (f) ZVND2 (f)

#elif __aarch64

#define _ZVNF1(f) SF1 (f) VF1 (f)
#define _ZVND1(f) SD1 (f) VD1 (f)
#define _ZVNF2(f) SF2 (f) VF2 (f)
#define _ZVND2(f) SD2 (f) VD2 (f)

#else

#define _ZVNF1(f) SF1 (f)
#define _ZVND1(f) SD1 (f)
#define _ZVNF2(f) SF2 (f)
#define _ZVND2(f) SD2 (f)

#endif

#define _ZSVF1(f) SVF1 (f) ZSVF1 (f)
#define _ZSVF2(f) SVF2 (f) ZSVF2 (f)
#define _ZSVD1(f) SVD1 (f) ZSVD1 (f)
#define _ZSVD2(f) SVD2 (f) ZSVD2 (f)

F1 (acosh)
F1 (asinh)
F2 (atan2)
F1 (atanh)
F1 (cosh)
F1 (erfc)
F1 (erf)
F1 (expm1)
F1 (log10)
F1 (log1p)
F1 (sinh)
F1 (tan)
D1 (acosh)
D1 (asinh)
D2 (atan2)
D1 (cosh)
D1 (erfc)
D1 (expm1)
D1 (log10)
D1 (log1p)
D1 (sinh)
#if WANT_VMATH
_ZVNF1 (asinh)
_ZVNF1 (atan)
_ZVND1 (atan)
_ZVNF2 (atan2)
_ZVND2 (atan2)
_ZVNF1 (atanh)
_ZVNF1 (cosh)
_ZVND1 (cosh)
_ZVNF1 (erf)
_ZVND1 (erf)
_ZVNF1 (erfc)
_ZVND1 (erfc)
_ZVNF1 (expm1)
_ZVND1 (expm1)
_ZVNF1 (log10)
_ZVND1 (log10)
_ZVNF1 (log1p)
_ZVND1 (log1p)
_ZVNF1 (log2)
_ZVND1 (log2)
_ZVNF1 (sinh)
_ZVND1 (sinh)
_ZVNF1 (tan)
#if WANT_SVE_MATH
_ZSVF2 (atan2)
_ZSVD2 (atan2)
_ZSVF1 (atan)
_ZSVD1 (atan)
_ZSVF1 (cos)
_ZSVD1 (cos)
_ZSVF1 (erf)
_ZSVD1 (erf)
_ZSVD1 (erfc)
_ZSVF1 (exp)
_ZSVF1 (log)
_ZSVD1 (log)
_ZSVF1 (log10)
_ZSVD1 (log10)
F (__sv_powi, sv_powi, ref_powi, mpfr_powi, 2, 0, d2, 0)
F (_ZGVsMxvv_powk, Z_sv_powk, ref_powi, mpfr_powi, 2, 0, d2, 0)
F (__sv_powif, sv_powif, ref_powif, mpfr_powi, 2, 1, f2, 0)
F (_ZGVsMxvv_powi, Z_sv_powi, ref_powif, mpfr_powi, 2, 1, f2, 0)
_ZSVF1 (sin)
_ZSVD1 (sin)
_ZSVF1 (tan)
#endif
#endif
