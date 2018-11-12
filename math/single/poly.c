/*
 * poly.c
 *
 * Copyright (c) 1998-2018, Arm Limited.
 * SPDX-License-Identifier: MIT
 */

double __kernel_poly(const double *coeffs, int n, double x)
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
