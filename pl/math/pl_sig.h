/*
 * PL macros for emitting various ulp/bench entries based on function signature
 *
 * Copyright (c) 2022-2024, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception.
 */

/* If streaming-compatibility is enabled, paste over instances of SV with SC to
   ensure that the source including this file emits names and signatures
   corresponding to the _sc_ variant. This requires one level of deferral in
   expansion of PL_SIG.  */
#if ENABLE_SC_COMPAT
# define SC_NAME_F1(fun) _ZGVsMxv_sc_##fun##f
# define SC_NAME_D1(fun) _ZGVsMxv_sc_##fun
# define SC_NAME_F2(fun) _ZGVsMxvv_sc_##fun##f
# define SC_NAME_D2(fun) _ZGVsMxvv_sc_##fun
# define SV SC
# define SV_NAME_F1 SC_NAME_F1
# define SV_NAME_D1 SC_NAME_D1
# define SV_NAME_F2 SC_NAME_F2
# define SV_NAME_D2 SC_NAME_D2
#else
# define SV_NAME_F1(fun) _ZGVsMxv_##fun##f
# define SV_NAME_D1(fun) _ZGVsMxv_##fun
# define SV_NAME_F2(fun) _ZGVsMxvv_##fun##f
# define SV_NAME_D2(fun) _ZGVsMxvv_##fun
#endif

#define PL_DECL_SF1(fun) float fun##f (float);
#define PL_DECL_SF2(fun) float fun##f (float, float);
#define PL_DECL_SD1(fun) double fun (double);
#define PL_DECL_SD2(fun) double fun (double, double);

#if WANT_VMATH
# define V_NAME_F1(fun) _ZGVnN4v_##fun##f
# define V_NAME_D1(fun) _ZGVnN2v_##fun
# define V_NAME_F2(fun) _ZGVnN4vv_##fun##f
# define V_NAME_D2(fun) _ZGVnN2vv_##fun
# define PL_DECL_VF1(fun)                                                    \
    VPCS_ATTR float32x4_t V_NAME_F1 (fun##f) (float32x4_t);
# define PL_DECL_VF2(fun)                                                    \
    VPCS_ATTR float32x4_t V_NAME_F2 (fun##f) (float32x4_t, float32x4_t);
# define PL_DECL_VD1(fun) VPCS_ATTR float64x2_t V_NAME_D1 (fun) (float64x2_t);
# define PL_DECL_VD2(fun)                                                    \
    VPCS_ATTR float64x2_t V_NAME_D2 (fun) (float64x2_t, float64x2_t);
#endif

#if WANT_SVE_MATH
# define PL_DECL_SVF1(fun)                                                   \
    svfloat32_t SV_NAME_F1 (fun) (svfloat32_t, svbool_t);
# define PL_DECL_SVF2(fun)                                                   \
    svfloat32_t SV_NAME_F2 (fun) (svfloat32_t, svfloat32_t, svbool_t);
# define PL_DECL_SVD1(fun)                                                   \
    svfloat64_t SV_NAME_D1 (fun) (svfloat64_t, svbool_t);
# define PL_DECL_SVD2(fun)                                                   \
    svfloat64_t SV_NAME_D2 (fun) (svfloat64_t, svfloat64_t, svbool_t);

# if WANT_SME_MATH
#  define PL_DECL_SCF1(fun)                                                   \
      svfloat32_t SC_NAME_F1 (fun) (svfloat32_t, svbool_t) SC_ATTR;
#  define PL_DECL_SCF2(fun)                                                   \
      svfloat32_t SC_NAME_F2 (fun) (svfloat32_t, svfloat32_t, svbool_t)       \
	  SC_ATTR;
#  define PL_DECL_SCD1(fun)                                                   \
      svfloat64_t SC_NAME_D1 (fun) (svfloat64_t, svbool_t) SC_ATTR;
#  define PL_DECL_SCD2(fun)                                                   \
      svfloat64_t SC_NAME_D2 (fun) (svfloat64_t, svfloat64_t, svbool_t)       \
	  SC_ATTR;
# endif
#endif

/* For building the routines, emit function prototype from PL_SIG. This
   ensures that the correct signature has been chosen (wrong one will be a
   compile error). PL_SIG is defined differently by various components of the
   build system to emit entries in the wrappers and entries for mathbench and
   ulp.  */
#ifndef _PL_SIG
# if defined(EMIT_ULP_FUNCS)
#  define _PL_SIG(v, t, a, f, ...) PL_SIG _Z##v##t##a (f)
# elif defined(EMIT_ULP_WRAPPERS)
#  define _PL_SIG(v, t, a, f, ...) PL_SIG Z##v##N##t##a##_WRAP (f)
# elif defined(EMIT_MATHBENCH_FUNCS)
#  define _PL_SIG(v, t, a, f, ...) PL_SIG _Z##v##t##a (f, ##__VA_ARGS__)
# else
#  define _PL_SIG(v, t, a, f, ...) PL_DECL_##v##t##a (f)
# endif
#endif

#define PL_SIG(...) _PL_SIG (__VA_ARGS__)
