/*
 * types.h
 *
 * Copyright (c) 2005-2015, Arm Limited.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef mathtest_types_h
#define mathtest_types_h

#include <limits.h>

#if UINT_MAX == 4294967295
typedef unsigned int uint32;
typedef int int32;
#define I32 ""
#elif ULONG_MAX == 4294967295
typedef unsigned long uint32;
typedef long int32;
#define I32 "l"
#else
#error Could not find an unsigned 32-bit integer type
#endif

#endif
