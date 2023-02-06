/*
 * Helper macros for double-precision Estrin polynomial evaluation
 * in SVE routines.
 *
 * Copyright (c) 2023, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

// clang-format off
#define  ESTRIN_1_(pg, x,                  c, i) FMA(pg, c(1 + i),                            x,   c(i))
#define  ESTRIN_2_(pg, x, x2,              c, i) FMA(pg, c(2 + i),                            x2,  ESTRIN_1_(pg, x,              c, i))
#define  ESTRIN_3_(pg, x, x2,              c, i) FMA(pg, ESTRIN_1_(pg, x,         c,  2 + i), x2,  ESTRIN_1_(pg, x,              c, i))
#define  ESTRIN_4_(pg, x, x2, x4,          c, i) FMA(pg, c(4 + i),   x4,                           ESTRIN_3_(pg, x, x2,          c, i))
#define  ESTRIN_5_(pg, x, x2, x4,          c, i) FMA(pg, ESTRIN_1_(pg, x,         c,  4 + i), x4,  ESTRIN_3_(pg, x, x2,          c, i))
#define  ESTRIN_6_(pg, x, x2, x4,          c, i) FMA(pg, ESTRIN_2_(pg, x, x2,     c,  4 + i), x4,  ESTRIN_3_(pg, x, x2,          c, i))
#define  ESTRIN_7_(pg, x, x2, x4,          c, i) FMA(pg, ESTRIN_3_(pg, x, x2,     c,  4 + i), x4,  ESTRIN_3_(pg, x, x2,          c, i))
#define  ESTRIN_8_(pg, x, x2, x4, x8,      c, i) FMA(pg, c(8 + i),                            x8,  ESTRIN_7_(pg, x, x2, x4,      c, i))
#define  ESTRIN_9_(pg, x, x2, x4, x8,      c, i) FMA(pg, ESTRIN_1_(pg, x,         c,  8 + i), x8,  ESTRIN_7_(pg, x, x2, x4,      c, i))
#define ESTRIN_10_(pg, x, x2, x4, x8,      c, i) FMA(pg, ESTRIN_2_(pg, x, x2,     c,  8 + i), x8,  ESTRIN_7_(pg, x, x2, x4,      c, i))
#define ESTRIN_11_(pg, x, x2, x4, x8,      c, i) FMA(pg, ESTRIN_3_(pg, x, x2,     c,  8 + i), x8,  ESTRIN_7_(pg, x, x2, x4,      c, i))
#define ESTRIN_12_(pg, x, x2, x4, x8,      c, i) FMA(pg, ESTRIN_4_(pg, x, x2, x4, c,  8 + i), x8,  ESTRIN_7_(pg, x, x2, x4,      c, i))
#define ESTRIN_13_(pg, x, x2, x4, x8,      c, i) FMA(pg, ESTRIN_5_(pg, x, x2, x4, c,  8 + i), x8,  ESTRIN_7_(pg, x, x2, x4,      c, i))
#define ESTRIN_14_(pg, x, x2, x4, x8,      c, i) FMA(pg, ESTRIN_6_(pg, x, x2, x4, c,  8 + i), x8,  ESTRIN_7_(pg, x, x2, x4,      c, i))
#define ESTRIN_15_(pg, x, x2, x4, x8,      c, i) FMA(pg, ESTRIN_7_(pg, x, x2, x4, c,  8 + i), x8,  ESTRIN_7_(pg, x, x2, x4,      c, i))
#define ESTRIN_16_(pg, x, x2, x4, x8, x16, c, i) FMA(pg, c(16 + i),                           x16, ESTRIN_15_(pg, x, x2, x4, x8, c, i))
#define ESTRIN_17_(pg, x, x2, x4, x8, x16, c, i) FMA(pg, ESTRIN_1_(pg, x,         c, 16 + i), x16, ESTRIN_15_(pg, x, x2, x4, x8, c, i))
#define ESTRIN_18_(pg, x, x2, x4, x8, x16, c, i) FMA(pg, ESTRIN_2_(pg, x, x2,     c, 16 + i), x16, ESTRIN_15_(pg, x, x2, x4, x8, c, i))
#define ESTRIN_19_(pg, x, x2, x4, x8, x16, c, i) FMA(pg, ESTRIN_3_(pg, x, x2,     c, 16 + i), x16, ESTRIN_15_(pg, x, x2, x4, x8, c, i))

#define  ESTRIN_1(pg, x,                  c)  ESTRIN_1_(pg, x,                  c, 0)
#define  ESTRIN_2(pg, x, x2,              c)  ESTRIN_2_(pg, x, x2,              c, 0)
#define  ESTRIN_3(pg, x, x2,              c)  ESTRIN_3_(pg, x, x2,              c, 0)
#define  ESTRIN_4(pg, x, x2, x4,          c)  ESTRIN_4_(pg, x, x2, x4,          c, 0)
#define  ESTRIN_5(pg, x, x2, x4,          c)  ESTRIN_5_(pg, x, x2, x4,          c, 0)
#define  ESTRIN_6(pg, x, x2, x4,          c)  ESTRIN_6_(pg, x, x2, x4,          c, 0)
#define  ESTRIN_7(pg, x, x2, x4,          c)  ESTRIN_7_(pg, x, x2, x4,          c, 0)
#define  ESTRIN_8(pg, x, x2, x4, x8,      c)  ESTRIN_8_(pg, x, x2, x4, x8,      c, 0)
#define  ESTRIN_9(pg, x, x2, x4, x8,      c)  ESTRIN_9_(pg, x, x2, x4, x8,      c, 0)
#define ESTRIN_10(pg, x, x2, x4, x8,      c) ESTRIN_10_(pg, x, x2, x4, x8,      c, 0)
#define ESTRIN_11(pg, x, x2, x4, x8,      c) ESTRIN_11_(pg, x, x2, x4, x8,      c, 0)
#define ESTRIN_12(pg, x, x2, x4, x8,      c) ESTRIN_12_(pg, x, x2, x4, x8,      c, 0)
#define ESTRIN_13(pg, x, x2, x4, x8,      c) ESTRIN_13_(pg, x, x2, x4, x8,      c, 0)
#define ESTRIN_14(pg, x, x2, x4, x8,      c) ESTRIN_14_(pg, x, x2, x4, x8,      c, 0)
#define ESTRIN_15(pg, x, x2, x4, x8,      c) ESTRIN_15_(pg, x, x2, x4, x8,      c, 0)
#define ESTRIN_16(pg, x, x2, x4, x8, x16, c) ESTRIN_16_(pg, x, x2, x4, x8, x16, c, 0)
#define ESTRIN_17(pg, x, x2, x4, x8, x16, c) ESTRIN_17_(pg, x, x2, x4, x8, x16, c, 0)
#define ESTRIN_18(pg, x, x2, x4, x8, x16, c) ESTRIN_18_(pg, x, x2, x4, x8, x16, c, 0)
#define ESTRIN_19(pg, x, x2, x4, x8, x16, c) ESTRIN_19_(pg, x, x2, x4, x8, x16, c, 0)
// clang-format on
