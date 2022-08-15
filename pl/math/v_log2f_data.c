/*
 * Coefficients and table entries for vector log2f
 *
 * Copyright (c) 2022, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "math_config.h"

const struct v_log2f_data __v_log2f_data = {
/* All values here are derived from the values in math/log2f_data.c.
   For all i:
     tab[i].invc_hi = (float) log2f_data.invc
     tab[i].invc_lo = log2f_data.invc - (double) tab[i].invc_hi
     tab[i].logc    = (float) log2f_data.logc
     poly[i]        = (float) log2f_data.poly[i].  */
  .tab = {
  { 0x1.661ec8p+0,  -0x1.81c31p-26,  -0x1.efec66p-2, 0},
  { 0x1.571ed4p+0,   0x1.55f108p-25, -0x1.b0b684p-2, 0},
  { 0x1.4953ap+0,   -0x1.e1fdeap-25, -0x1.7418bp-2, 0},
  { 0x1.3c995cp+0,  -0x1.e8ff9p-25,  -0x1.39de92p-2, 0},
  { 0x1.30d19p+0,    0x1.910c94p-25, -0x1.01d9cp-2, 0},
  { 0x1.25e228p+0,  -0x1.3d1c58p-26, -0x1.97c1d2p-3, 0},
  { 0x1.1bb4a4p+0,   0x1.434688p-25, -0x1.2f9e3ap-3, 0},
  { 0x1.12359p+0,   -0x1.eea348p-25, -0x1.960cbcp-4, 0},
  { 0x1.0953f4p+0,   0x1.9900a8p-28, -0x1.a6f9dcp-5, 0},
  { 0x1p+0,          0x0p+0,          0x0p+0, 0},
  { 0x1.e608dp-1,   -0x1.32dc2ap-28,  0x1.338caap-4, 0},
  { 0x1.ca4b32p-1,  -0x1.fb2acp-30,   0x1.476a96p-3, 0},
  { 0x1.b20366p-1,  -0x1.12a064p-26,  0x1.e840b4p-3, 0},
  { 0x1.9c2d16p-1,   0x1.d0d516p-28,  0x1.40646p-2, 0},
  { 0x1.886e6p-1,    0x1.bc20f6p-28,  0x1.88e9c2p-2, 0},
  { 0x1.767ddp-1,   -0x1.5596f4p-26,  0x1.ce0a44p-2, 0},
  },
  .poly = { -0x1.712b70p-2, 0x1.ecabf4p-2,
	    -0x1.71547ap-1, 0x1.715476p+0 }
};
