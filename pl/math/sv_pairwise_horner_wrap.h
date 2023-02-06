/*
 * Helper macros for pairwise Horner polynomial evaluation in SVE routines.
 *
 * Copyright (c) 2023, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

// clang-format off
#define  PW_HORNER_1_(pg, x, c,     i) FMA(pg, VECTOR(c(i + 1)),                   x,               VECTOR(c(i)))
#define  PW_HORNER_3_(pg, x, x2, c, i) FMA(pg, PW_HORNER_1_ (pg, x,     c, i + 2), x2, PW_HORNER_1_(pg, x, c, i))
#define  PW_HORNER_5_(pg, x, x2, c, i) FMA(pg, PW_HORNER_3_ (pg, x, x2, c, i + 2), x2, PW_HORNER_1_(pg, x, c, i))
#define  PW_HORNER_7_(pg, x, x2, c, i) FMA(pg, PW_HORNER_5_ (pg, x, x2, c, i + 2), x2, PW_HORNER_1_(pg, x, c, i))
#define  PW_HORNER_9_(pg, x, x2, c, i) FMA(pg, PW_HORNER_7_ (pg, x, x2, c, i + 2), x2, PW_HORNER_1_(pg, x, c, i))
#define PW_HORNER_11_(pg, x, x2, c, i) FMA(pg, PW_HORNER_9_ (pg, x, x2, c, i + 2), x2, PW_HORNER_1_(pg, x, c, i))
#define PW_HORNER_13_(pg, x, x2, c, i) FMA(pg, PW_HORNER_11_(pg, x, x2, c, i + 2), x2, PW_HORNER_1_(pg, x, c, i))
#define PW_HORNER_15_(pg, x, x2, c, i) FMA(pg, PW_HORNER_13_(pg, x, x2, c, i + 2), x2, PW_HORNER_1_(pg, x, c, i))
#define PW_HORNER_17_(pg, x, x2, c, i) FMA(pg, PW_HORNER_15_(pg, x, x2, c, i + 2), x2,  PW_HORNER_1_(pg, x, c, i))

#define  PAIRWISE_HORNER_1(pg, x,     c) PW_HORNER_1_ (pg, x, c, 0)
#define  PAIRWISE_HORNER_3(pg, x, x2, c) PW_HORNER_3_ (pg, x, x2, c, 0)
#define  PAIRWISE_HORNER_5(pg, x, x2, c) PW_HORNER_5_ (pg, x, x2, c, 0)
#define  PAIRWISE_HORNER_7(pg, x, x2, c) PW_HORNER_7_ (pg, x, x2, c, 0)
#define  PAIRWISE_HORNER_9(pg, x, x2, c) PW_HORNER_9_ (pg, x, x2, c, 0)
#define PAIRWISE_HORNER_11(pg, x, x2, c) PW_HORNER_11_(pg, x, x2, c, 0)
#define PAIRWISE_HORNER_13(pg, x, x2, c) PW_HORNER_13_(pg, x, x2, c, 0)
#define PAIRWISE_HORNER_15(pg, x, x2, c) PW_HORNER_15_(pg, x, x2, c, 0)
#define PAIRWISE_HORNER_17(pg, x, x2, c) PW_HORNER_17_(pg, x, x2, c, 0)

#define  PW_HORNER_2_(pg, x, x2, c, i) FMA(pg, VECTOR(c(i + 2)),                   x2, PW_HORNER_1_(pg, x, c, i))
#define  PW_HORNER_4_(pg, x, x2, c, i) FMA(pg, PW_HORNER_2_ (pg, x, x2, c, i + 2), x2, PW_HORNER_1_(pg, x, c, i))
#define  PW_HORNER_6_(pg, x, x2, c, i) FMA(pg, PW_HORNER_4_ (pg, x, x2, c, i + 2), x2, PW_HORNER_1_(pg, x, c, i))
#define  PW_HORNER_8_(pg, x, x2, c, i) FMA(pg, PW_HORNER_6_ (pg, x, x2, c, i + 2), x2, PW_HORNER_1_(pg, x, c, i))
#define PW_HORNER_10_(pg, x, x2, c, i) FMA(pg, PW_HORNER_8_ (pg, x, x2, c, i + 2), x2, PW_HORNER_1_(pg, x, c, i))
#define PW_HORNER_12_(pg, x, x2, c, i) FMA(pg, PW_HORNER_10_(pg, x, x2, c, i + 2), x2, PW_HORNER_1_(pg, x, c, i))
#define PW_HORNER_14_(pg, x, x2, c, i) FMA(pg, PW_HORNER_12_(pg, x, x2, c, i + 2), x2, PW_HORNER_1_(pg, x, c, i))
#define PW_HORNER_16_(pg, x, x2, c, i) FMA(pg, PW_HORNER_14_(pg, x, x2, c, i + 2), x2, PW_HORNER_1_(pg, x, c, i))
#define PW_HORNER_18_(pg, x, x2, c, i) FMA(pg, PW_HORNER_16_(pg, x, x2, c, i + 2), x2, PW_HORNER_1_(pg, x, c, i))

#define  PAIRWISE_HORNER_2(pg, x, x2, c) PW_HORNER_2_ (pg, x, x2, c, 0)
#define  PAIRWISE_HORNER_4(pg, x, x2, c) PW_HORNER_4_ (pg, x, x2, c, 0)
#define  PAIRWISE_HORNER_6(pg, x, x2, c) PW_HORNER_6_ (pg, x, x2, c, 0)
#define  PAIRWISE_HORNER_8(pg, x, x2, c) PW_HORNER_8_(pg, x, x2, c, 0)
#define PAIRWISE_HORNER_10(pg, x, x2, c) PW_HORNER_10_(pg, x, x2, c, 0)
#define PAIRWISE_HORNER_12(pg, x, x2, c) PW_HORNER_12_(pg, x, x2, c, 0)
#define PAIRWISE_HORNER_14(pg, x, x2, c) PW_HORNER_14_(pg, x, x2, c, 0)
#define PAIRWISE_HORNER_16(pg, x, x2, c) PW_HORNER_16_(pg, x, x2, c, 0)
#define PAIRWISE_HORNER_18(pg, x, x2, c) PW_HORNER_18_(pg, x, x2, c, 0)
// clang-format on
