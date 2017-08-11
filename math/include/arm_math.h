/*
 *  arm_math.h - ARM math library function definitions
 *
 *  Copyright (C) 2015, ARM Limited, All Rights Reserved
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

/* Double precision */
__attribute__((const)) double ARM__sin(double);
__attribute__((const)) double ARM__cos(double);
double ARM__tan(double);

/* Single precision */
float ARM__sinf(float);
float ARM__cosf(float);
float ARM__tanf(float);
float ARM__expf(float);
float ARM__exp2f(float);
float ARM__logf(float);
float ARM__log2f(float);
float ARM__powf(float, float);
