/*
 * Helper macros for Horner polynomial evaluation in SVE routines.
 *
 * Copyright (c) 2023, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

// clang-format off
#define  HORNER_1_(pg, x, c, i) FMA(pg, VECTOR(c(i + 1)), x,    VECTOR(c(i)))
#define  HORNER_2_(pg, x, c, i) FMA(pg, HORNER_1_ (pg, x, c, i + 1), x, VECTOR(c(i)))
#define  HORNER_3_(pg, x, c, i) FMA(pg, HORNER_2_ (pg, x, c, i + 1), x, VECTOR(c(i)))
#define  HORNER_4_(pg, x, c, i) FMA(pg, HORNER_3_ (pg, x, c, i + 1), x, VECTOR(c(i)))
#define  HORNER_5_(pg, x, c, i) FMA(pg, HORNER_4_ (pg, x, c, i + 1), x, VECTOR(c(i)))
#define  HORNER_6_(pg, x, c, i) FMA(pg, HORNER_5_ (pg, x, c, i + 1), x, VECTOR(c(i)))
#define  HORNER_7_(pg, x, c, i) FMA(pg, HORNER_6_ (pg, x, c, i + 1), x, VECTOR(c(i)))
#define  HORNER_8_(pg, x, c, i) FMA(pg, HORNER_7_ (pg, x, c, i + 1), x, VECTOR(c(i)))
#define  HORNER_9_(pg, x, c, i) FMA(pg, HORNER_8_ (pg, x, c, i + 1), x, VECTOR(c(i)))
#define HORNER_10_(pg, x, c, i) FMA(pg, HORNER_9_ (pg, x, c, i + 1), x, VECTOR(c(i)))
#define HORNER_11_(pg, x, c, i) FMA(pg, HORNER_10_(pg, x, c, i + 1), x, VECTOR(c(i)))
#define HORNER_12_(pg, x, c, i) FMA(pg, HORNER_11_(pg, x, c, i + 1), x, VECTOR(c(i)))

#define  HORNER_1(pg, x, c) HORNER_1_ (pg, x, c, 0)
#define  HORNER_2(pg, x, c) HORNER_2_ (pg, x, c, 0)
#define  HORNER_3(pg, x, c) HORNER_3_ (pg, x, c, 0)
#define  HORNER_4(pg, x, c) HORNER_4_ (pg, x, c, 0)
#define  HORNER_5(pg, x, c) HORNER_5_ (pg, x, c, 0)
#define  HORNER_6(pg, x, c) HORNER_6_ (pg, x, c, 0)
#define  HORNER_7(pg, x, c) HORNER_7_ (pg, x, c, 0)
#define  HORNER_8(pg, x, c) HORNER_8_ (pg, x, c, 0)
#define  HORNER_9(pg, x, c) HORNER_9_ (pg, x, c, 0)
#define HORNER_10(pg, x, c) HORNER_10_(pg, x, c, 0)
#define HORNER_11(pg, x, c) HORNER_11_(pg, x, c, 0)
#define HORNER_12(pg, x, c) HORNER_12_(pg, x, c, 0)
// clang-format on
