/*
 *  semi.h: header for semi.c
 *
 *  Copyright (C) 1999-2015, ARM Limited, All Rights Reserved
 *  SPDX-License-Identifier: Apache-2.0
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may
 *  not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  This file is part of the Optimized Routines project
 */

#ifndef test_semi_h
#define test_semi_h

#include "types.h"

char *test_ceil(uint32 *in, uint32 *out);
char *test_floor(uint32 *in, uint32 *out);
char *test_fmod(uint32 *a, uint32 *b, uint32 *out);
char *test_ldexp(uint32 *x, uint32 *n, uint32 *out);
char *test_frexp(uint32 *x, uint32 *out, uint32 *nout);
char *test_modf(uint32 *x, uint32 *iout, uint32 *fout);
char *test_ceilf(uint32 *in, uint32 *out);
char *test_floorf(uint32 *in, uint32 *out);
char *test_fmodf(uint32 *a, uint32 *b, uint32 *out);
char *test_ldexpf(uint32 *x, uint32 *n, uint32 *out);
char *test_frexpf(uint32 *x, uint32 *out, uint32 *nout);
char *test_modff(uint32 *x, uint32 *iout, uint32 *fout);

char *test_copysign(uint32 *x, uint32 *y, uint32 *out);
char *test_copysignf(uint32 *x, uint32 *y, uint32 *out);
char *test_isfinite(uint32 *x, uint32 *out);
char *test_isfinitef(uint32 *x, uint32 *out);
char *test_isinf(uint32 *x, uint32 *out);
char *test_isinff(uint32 *x, uint32 *out);
char *test_isnan(uint32 *x, uint32 *out);
char *test_isnanf(uint32 *x, uint32 *out);
char *test_isnormal(uint32 *x, uint32 *out);
char *test_isnormalf(uint32 *x, uint32 *out);
char *test_signbit(uint32 *x, uint32 *out);
char *test_signbitf(uint32 *x, uint32 *out);
char *test_fpclassify(uint32 *x, uint32 *out);
char *test_fpclassifyf(uint32 *x, uint32 *out);

char *test_isgreater(uint32 *x, uint32 *y, uint32 *out);
char *test_isgreaterequal(uint32 *x, uint32 *y, uint32 *out);
char *test_isless(uint32 *x, uint32 *y, uint32 *out);
char *test_islessequal(uint32 *x, uint32 *y, uint32 *out);
char *test_islessgreater(uint32 *x, uint32 *y, uint32 *out);
char *test_isunordered(uint32 *x, uint32 *y, uint32 *out);
char *test_isgreaterf(uint32 *x, uint32 *y, uint32 *out);
char *test_isgreaterequalf(uint32 *x, uint32 *y, uint32 *out);
char *test_islessf(uint32 *x, uint32 *y, uint32 *out);
char *test_islessequalf(uint32 *x, uint32 *y, uint32 *out);
char *test_islessgreaterf(uint32 *x, uint32 *y, uint32 *out);
char *test_isunorderedf(uint32 *x, uint32 *y, uint32 *out);
#endif
