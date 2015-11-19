/*
 *  poly.c
 *
 *  Copyright (C) 1998-2015, ARM Limited, All Rights Reserved
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

double ARM__kernel_poly(const double *coeffs, int n, double x)
{
  double result = coeffs[--n];

  while ((n & ~0x6) != 0)         /* Loop until n even and < 8 */
    result = (result * x) + coeffs[--n];

  switch (n)
    {
    case 6: result = (result * x) + coeffs[5];
      result = (result * x) + coeffs[4];
    case 4: result = (result * x) + coeffs[3];
      result = (result * x) + coeffs[2];
    case 2: result = (result * x) + coeffs[1];
      result = (result * x) + coeffs[0];
    }
  return result;
}
