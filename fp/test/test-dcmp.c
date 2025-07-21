/*
 * Tests of IEEE 754 double-precision division
 *
 * Copyright (c) 1999-2025, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include <inttypes.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

enum FloatCompareResult {
  FCR_LT, FCR_EQ, FCR_GT, FCR_UN,
};

static const char *const result_strings[] = {
  "less", "equal", "greater", "unordered",
};

struct test
{
  uint64_t in1, in2;
  enum FloatCompareResult out;
};

static const struct test tests[] = {
  { 0x0000000000000000, 0x0000000000000001, FCR_LT },
  { 0x0000000000000000, 0x000fffffffffffff, FCR_LT },
  { 0x0000000000000000, 0x3ff0000000000000, FCR_LT },
  { 0x0000000000000000, 0x7fe0000000000000, FCR_LT },
  { 0x0000000000000000, 0x7ff0000000000000, FCR_LT },
  { 0x0000000000000000, 0x7ff00000a5a42e09, FCR_UN },
  { 0x0000000000000000, 0x7ffcd5b95f9b89ae, FCR_UN },
  { 0x0000000000000000, 0x7ffcd5b95f9b89ae, FCR_UN },
  { 0x0000000000000000, 0x8000000000000000, FCR_EQ },
  { 0x0000000000000000, 0x8000000000000001, FCR_GT },
  { 0x0000000000000000, 0x800fffffffffffff, FCR_GT },
  { 0x0000000000000000, 0x8010000000000000, FCR_GT },
  { 0x0000000000000000, 0xfff0000000000000, FCR_GT },
  { 0x0000000000000000, 0xfff00000a5a42e09, FCR_UN },
  { 0x0000000000000000, 0xfffcd5b95f9b89ae, FCR_UN },
  { 0x0000000000000000, 0xfffcd5b95f9b89ae, FCR_UN },
  { 0x0000000000000001, 0x0000000000000001, FCR_EQ },
  { 0x0000000000000001, 0x3fefffffffffffff, FCR_LT },
  { 0x0000000000000001, 0x3ff0000000000000, FCR_LT },
  { 0x0000000000000001, 0x3ffffffffffffffe, FCR_LT },
  { 0x0000000000000001, 0x3fffffffffffffff, FCR_LT },
  { 0x0000000000000001, 0x7fdfffffffffffff, FCR_LT },
  { 0x0000000000000001, 0x7fe0000000000000, FCR_LT },
  { 0x0000000000000001, 0x7feffffffffffffe, FCR_LT },
  { 0x0000000000000001, 0x7fefffffffffffff, FCR_LT },
  { 0x0000000000000001, 0x7ff00000887bcf03, FCR_UN },
  { 0x0000000000000001, 0x7ff753b1887bcf03, FCR_UN },
  { 0x0000000000000001, 0x7ffc3134b058fe20, FCR_UN },
  { 0x0000000000000001, 0x8000000000000001, FCR_GT },
  { 0x0000000000000001, 0xbfefffffffffffff, FCR_GT },
  { 0x0000000000000001, 0xbff0000000000000, FCR_GT },
  { 0x0000000000000001, 0xbffffffffffffffe, FCR_GT },
  { 0x0000000000000001, 0xbfffffffffffffff, FCR_GT },
  { 0x0000000000000001, 0xffdfffffffffffff, FCR_GT },
  { 0x0000000000000001, 0xffe0000000000000, FCR_GT },
  { 0x0000000000000001, 0xffeffffffffffffe, FCR_GT },
  { 0x0000000000000001, 0xffefffffffffffff, FCR_GT },
  { 0x0000000000000001, 0xfff00000887bcf03, FCR_UN },
  { 0x0000000000000001, 0xfff753b1887bcf03, FCR_UN },
  { 0x0000000000000001, 0xfffc3134b058fe20, FCR_UN },
  { 0x0000000000000002, 0x0000000000000001, FCR_GT },
  { 0x0000000000000003, 0x0000000000000002, FCR_GT },
  { 0x0000000000000003, 0x4008000000000000, FCR_LT },
  { 0x0000000000000003, 0x4014000000000000, FCR_LT },
  { 0x0000000000000003, 0x7fe0000000000000, FCR_LT },
  { 0x0000000000000003, 0xc014000000000000, FCR_GT },
  { 0x0000000000000003, 0xffe0000000000000, FCR_GT },
  { 0x0000000000000004, 0x0000000000000004, FCR_EQ },
  { 0x000ffffffffffffc, 0x800ffffffffffffc, FCR_GT },
  { 0x000ffffffffffffd, 0x000ffffffffffffe, FCR_LT },
  { 0x000fffffffffffff, 0x0000000000000000, FCR_GT },
  { 0x000fffffffffffff, 0x000ffffffffffffe, FCR_GT },
  { 0x000fffffffffffff, 0x000fffffffffffff, FCR_EQ },
  { 0x000fffffffffffff, 0x0010000000000000, FCR_LT },
  { 0x000fffffffffffff, 0x7ff0000000000000, FCR_LT },
  { 0x000fffffffffffff, 0x7ff00000dfe15ee3, FCR_UN },
  { 0x000fffffffffffff, 0x7ff6d1ebdfe15ee3, FCR_UN },
  { 0x000fffffffffffff, 0x7ffed0664505a878, FCR_UN },
  { 0x000fffffffffffff, 0x8000000000000000, FCR_GT },
  { 0x000fffffffffffff, 0xfff0000000000000, FCR_GT },
  { 0x000fffffffffffff, 0xfff00000dfe15ee3, FCR_UN },
  { 0x000fffffffffffff, 0xfff6d1ebdfe15ee3, FCR_UN },
  { 0x000fffffffffffff, 0xfffed0664505a878, FCR_UN },
  { 0x0010000000000000, 0x0000000000000000, FCR_GT },
  { 0x0010000000000000, 0x0010000000000000, FCR_EQ },
  { 0x0010000000000000, 0x8010000000000000, FCR_GT },
  { 0x0010000000000001, 0x0010000000000000, FCR_GT },
  { 0x0010000000000001, 0x0010000000000002, FCR_LT },
  { 0x001fffffffffffff, 0x0020000000000000, FCR_LT },
  { 0x001fffffffffffff, 0x0020000000000002, FCR_LT },
  { 0x001fffffffffffff, 0x0020000000000004, FCR_LT },
  { 0x0020000000000000, 0x001fffffffffffff, FCR_GT },
  { 0x0020000000000001, 0x0010000000000001, FCR_GT },
  { 0x0020000000000001, 0x001fffffffffffff, FCR_GT },
  { 0x0020000000000002, 0x0010000000000001, FCR_GT },
  { 0x002fffffffffffff, 0x0030000000000000, FCR_LT },
  { 0x0030000000000000, 0x002fffffffffffff, FCR_GT },
  { 0x0030000000000001, 0x002fffffffffffff, FCR_GT },
  { 0x0030000000000002, 0x0020000000000003, FCR_GT },
  { 0x3fe0000000000000, 0x3fe0000000000000, FCR_EQ },
  { 0x3fefffffffffffff, 0x0000000000000001, FCR_GT },
  { 0x3fefffffffffffff, 0x8000000000000001, FCR_GT },
  { 0x3ff0000000000000, 0x3ff0000000000000, FCR_EQ },
  { 0x3ff0000000000000, 0x3ff0000000000003, FCR_LT },
  { 0x3ff0000000000000, 0x4000000000000000, FCR_LT },
  { 0x3ff0000000000000, 0x401c000000000000, FCR_LT },
  { 0x3ff0000000000000, 0x7ff0000033022725, FCR_UN },
  { 0x3ff0000000000000, 0x7ff4f5ad33022725, FCR_UN },
  { 0x3ff0000000000000, 0x7ffd3870667efc9d, FCR_UN },
  { 0x3ff0000000000000, 0x8000000000000000, FCR_GT },
  { 0x3ff0000000000000, 0xbff0000000000000, FCR_GT },
  { 0x3ff0000000000000, 0xbff0000000000003, FCR_GT },
  { 0x3ff0000000000000, 0xfff0000033022725, FCR_UN },
  { 0x3ff0000000000000, 0xfff4f5ad33022725, FCR_UN },
  { 0x3ff0000000000000, 0xfffd3870667efc9d, FCR_UN },
  { 0x3ff0000000000001, 0x3ff0000000000000, FCR_GT },
  { 0x3ff0000000000001, 0x3ff0000000000002, FCR_LT },
  { 0x3ff0000000000001, 0xbff0000000000000, FCR_GT },
  { 0x3ffffffffffffffc, 0x3ffffffffffffffd, FCR_LT },
  { 0x3fffffffffffffff, 0x0000000000000001, FCR_GT },
  { 0x3fffffffffffffff, 0x4000000000000000, FCR_LT },
  { 0x4000000000000000, 0x3ff0000000000000, FCR_GT },
  { 0x4000000000000000, 0x3fffffffffffffff, FCR_GT },
  { 0x4000000000000000, 0x4000000000000000, FCR_EQ },
  { 0x4000000000000000, 0x4000000000000001, FCR_LT },
  { 0x4000000000000000, 0xc000000000000000, FCR_GT },
  { 0x4000000000000000, 0xc000000000000001, FCR_GT },
  { 0x4000000000000000, 0xc014000000000000, FCR_GT },
  { 0x4000000000000001, 0x3ff0000000000001, FCR_GT },
  { 0x4000000000000001, 0x4000000000000002, FCR_LT },
  { 0x4000000000000001, 0xc000000000000002, FCR_GT },
  { 0x4000000000000002, 0x3ff0000000000001, FCR_GT },
  { 0x4000000000000002, 0x3ff0000000000003, FCR_GT },
  { 0x4000000000000004, 0x4000000000000003, FCR_GT },
  { 0x4008000000000000, 0x4008000000000000, FCR_EQ },
  { 0x400fffffffffffff, 0x400ffffffffffffe, FCR_GT },
  { 0x400fffffffffffff, 0x4010000000000002, FCR_LT },
  { 0x4010000000000001, 0x400fffffffffffff, FCR_GT },
  { 0x4014000000000000, 0x0000000000000000, FCR_GT },
  { 0x4014000000000000, 0x8000000000000000, FCR_GT },
  { 0x4014000000000000, 0xbff0000000000000, FCR_GT },
  { 0x4014000000000000, 0xc014000000000000, FCR_GT },
  { 0x7fb0000000000001, 0x7fafffffffffffff, FCR_GT },
  { 0x7fcfffffffffffff, 0x7fcffffffffffffe, FCR_GT },
  { 0x7fcfffffffffffff, 0x7fd0000000000002, FCR_LT },
  { 0x7fd0000000000000, 0x7fcfffffffffffff, FCR_GT },
  { 0x7fd0000000000000, 0x7fd0000000000000, FCR_EQ },
  { 0x7fd0000000000000, 0x7fd0000000000001, FCR_LT },
  { 0x7fd0000000000001, 0x7fd0000000000000, FCR_GT },
  { 0x7fd0000000000001, 0x7fe0000000000001, FCR_LT },
  { 0x7fd0000000000001, 0xffd0000000000000, FCR_GT },
  { 0x7fd0000000000002, 0x7fc0000000000003, FCR_GT },
  { 0x7fd0000000000004, 0x7fd0000000000003, FCR_GT },
  { 0x7fdffffffffffffe, 0x7fdffffffffffffe, FCR_EQ },
  { 0x7fdffffffffffffe, 0x7fdfffffffffffff, FCR_LT },
  { 0x7fdffffffffffffe, 0xffdfffffffffffff, FCR_GT },
  { 0x7fdfffffffffffff, 0x3ff0000000000000, FCR_GT },
  { 0x7fdfffffffffffff, 0x7fe0000000000000, FCR_LT },
  { 0x7fdfffffffffffff, 0xbff0000000000000, FCR_GT },
  { 0x7fdfffffffffffff, 0xffe0000000000000, FCR_GT },
  { 0x7fe0000000000000, 0x3ff0000000000000, FCR_GT },
  { 0x7fe0000000000000, 0x7fe0000000000000, FCR_EQ },
  { 0x7fe0000000000000, 0x7ff0000000000000, FCR_LT },
  { 0x7fe0000000000000, 0xbff0000000000000, FCR_GT },
  { 0x7fe0000000000000, 0xffe0000000000000, FCR_GT },
  { 0x7fe0000000000000, 0xfff0000000000000, FCR_GT },
  { 0x7fe0000000000001, 0x7fe0000000000000, FCR_GT },
  { 0x7fe0000000000001, 0x7fe0000000000002, FCR_LT },
  { 0x7fe0000000000001, 0xffe0000000000000, FCR_GT },
  { 0x7fe0000000000002, 0x7fd0000000000001, FCR_GT },
  { 0x7feffffffffffffe, 0x3ff0000000000000, FCR_GT },
  { 0x7feffffffffffffe, 0x7fefffffffffffff, FCR_LT },
  { 0x7feffffffffffffe, 0xbff0000000000000, FCR_GT },
  { 0x7feffffffffffffe, 0xffefffffffffffff, FCR_GT },
  { 0x7fefffffffffffff, 0x0000000000000001, FCR_GT },
  { 0x7fefffffffffffff, 0x3ff0000000000000, FCR_GT },
  { 0x7fefffffffffffff, 0x7fefffffffffffff, FCR_EQ },
  { 0x7fefffffffffffff, 0x7ff00000c901461b, FCR_UN },
  { 0x7fefffffffffffff, 0x7ff784a9c901461b, FCR_UN },
  { 0x7fefffffffffffff, 0x7ffe2c1db2e4a313, FCR_UN },
  { 0x7fefffffffffffff, 0x8000000000000001, FCR_GT },
  { 0x7fefffffffffffff, 0xbff0000000000000, FCR_GT },
  { 0x7fefffffffffffff, 0xfff00000c901461b, FCR_UN },
  { 0x7fefffffffffffff, 0xfff784a9c901461b, FCR_UN },
  { 0x7fefffffffffffff, 0xfffe2c1db2e4a313, FCR_UN },
  { 0x7ff0000000000000, 0x0000000000000000, FCR_GT },
  { 0x7ff0000000000000, 0x0000000000000001, FCR_GT },
  { 0x7ff0000000000000, 0x000fffffffffffff, FCR_GT },
  { 0x7ff0000000000000, 0x7fe0000000000000, FCR_GT },
  { 0x7ff0000000000000, 0x7fefffffffffffff, FCR_GT },
  { 0x7ff0000000000000, 0x7ff0000000000000, FCR_EQ },
  { 0x7ff0000000000000, 0x7ff0e6d059ac9171, FCR_UN },
  { 0x7ff0000000000000, 0x7ffbda2fc9024ae6, FCR_UN },
  { 0x7ff0000000000000, 0x8000000000000000, FCR_GT },
  { 0x7ff0000000000000, 0x8000000000000001, FCR_GT },
  { 0x7ff0000000000000, 0x800fffffffffffff, FCR_GT },
  { 0x7ff0000000000000, 0xffe0000000000000, FCR_GT },
  { 0x7ff0000000000000, 0xffefffffffffffff, FCR_GT },
  { 0x7ff0000000000000, 0xfff0000000000000, FCR_GT },
  { 0x7ff0000047e8b9a0, 0x0000000000000000, FCR_UN },
  { 0x7ff4017647e8b9a0, 0x0000000000000000, FCR_UN },
  { 0x7ff00000abfe5d29, 0x0000000000000001, FCR_UN },
  { 0x7ff2a1cdabfe5d29, 0x0000000000000001, FCR_UN },
  { 0x7ff000005155db76, 0x000fffffffffffff, FCR_UN },
  { 0x7ff645cb5155db76, 0x000fffffffffffff, FCR_UN },
  { 0x7ff0000070c46aa0, 0x3ff0000000000000, FCR_UN },
  { 0x7ff2068470c46aa0, 0x3ff0000000000000, FCR_UN },
  { 0x7ff00000b5aee637, 0x7fefffffffffffff, FCR_UN },
  { 0x7ff72b19b5aee637, 0x7fefffffffffffff, FCR_UN },
  { 0x7ff00000c08c2788, 0x7ff0000000000000, FCR_UN },
  { 0x7ff1e0c1c08c2788, 0x7ff0000000000000, FCR_UN },
  { 0x7ff00000ec581a54, 0x7ff0000021ebdfaf, FCR_UN },
  { 0x7ff00000ec581a54, 0x7ff45d2221ebdfaf, FCR_UN },
  { 0x7ff571eaec581a54, 0x7ff0000021ebdfaf, FCR_UN },
  { 0x7ff571eaec581a54, 0x7ff45d2221ebdfaf, FCR_UN },
  { 0x7ff000003a3a1f94, 0x7ff00000229f3502, FCR_UN },
  { 0x7ff000003a3a1f94, 0x7ffb8fa0229f3502, FCR_UN },
  { 0x7ff6439e3a3a1f94, 0x7ff00000229f3502, FCR_UN },
  { 0x7ff6439e3a3a1f94, 0x7ffb8fa0229f3502, FCR_UN },
  { 0x7ff00000ec581a54, 0xfff0000021ebdfaf, FCR_UN },
  { 0x7ff00000ec581a54, 0xfff45d2221ebdfaf, FCR_UN },
  { 0x7ff571eaec581a54, 0xfff0000021ebdfaf, FCR_UN },
  { 0x7ff571eaec581a54, 0xfff45d2221ebdfaf, FCR_UN },
  { 0x7ff000003a3a1f94, 0xfff00000229f3502, FCR_UN },
  { 0x7ff000003a3a1f94, 0xfffb8fa0229f3502, FCR_UN },
  { 0x7ff6439e3a3a1f94, 0xfff00000229f3502, FCR_UN },
  { 0x7ff6439e3a3a1f94, 0xfffb8fa0229f3502, FCR_UN },
  { 0x7ff00000c31d528e, 0x8000000000000000, FCR_UN },
  { 0x7ff5fb72c31d528e, 0x8000000000000000, FCR_UN },
  { 0x7ff00000ac81d215, 0x8000000000000001, FCR_UN },
  { 0x7ff4481aac81d215, 0x8000000000000001, FCR_UN },
  { 0x7ff00000d12062fd, 0x800fffffffffffff, FCR_UN },
  { 0x7ff707f6d12062fd, 0x800fffffffffffff, FCR_UN },
  { 0x7ff000001c6481ef, 0xbff0000000000000, FCR_UN },
  { 0x7ff66ee91c6481ef, 0xbff0000000000000, FCR_UN },
  { 0x7ff00000985729a7, 0xffefffffffffffff, FCR_UN },
  { 0x7ff19cff985729a7, 0xffefffffffffffff, FCR_UN },
  { 0x7ff0000053ec80fe, 0xfff0000000000000, FCR_UN },
  { 0x7ff7dbc153ec80fe, 0xfff0000000000000, FCR_UN },
  { 0x7ff00000816fb493, 0x0000000000000000, FCR_UN },
  { 0x7ff87f75816fb493, 0x0000000000000000, FCR_UN },
  { 0x7ff000000c2d7c33, 0x0000000000000001, FCR_UN },
  { 0x7ff91ecb0c2d7c33, 0x0000000000000001, FCR_UN },
  { 0x7ff00000a68bae40, 0x000fffffffffffff, FCR_UN },
  { 0x7ffc0acda68bae40, 0x000fffffffffffff, FCR_UN },
  { 0x7ff000002fe14961, 0x3ff0000000000000, FCR_UN },
  { 0x7ffcfa4e2fe14961, 0x3ff0000000000000, FCR_UN },
  { 0x7ff000005c206da1, 0x7fefffffffffffff, FCR_UN },
  { 0x7ff800bb5c206da1, 0x7fefffffffffffff, FCR_UN },
  { 0x7ff0000051887a34, 0x7ff0000000000000, FCR_UN },
  { 0x7ffce11951887a34, 0x7ff0000000000000, FCR_UN },
  { 0x7ff000002b4c32a8, 0x7ff000001edb8786, FCR_UN },
  { 0x7ff000002b4c32a8, 0x7ff342ea1edb8786, FCR_UN },
  { 0x7ffbd6b52b4c32a8, 0x7ff000001edb8786, FCR_UN },
  { 0x7ffbd6b52b4c32a8, 0x7ff342ea1edb8786, FCR_UN },
  { 0x7ff00000bc88c2a9, 0x7ff000002fa062f4, FCR_UN },
  { 0x7ff00000bc88c2a9, 0x7ffdc9ee2fa062f4, FCR_UN },
  { 0x7ff8eaadbc88c2a9, 0x7ff000002fa062f4, FCR_UN },
  { 0x7ff8eaadbc88c2a9, 0x7ffdc9ee2fa062f4, FCR_UN },
  { 0x7ff000002b4c32a8, 0xfff000001edb8786, FCR_UN },
  { 0x7ff000002b4c32a8, 0xfff342ea1edb8786, FCR_UN },
  { 0x7ffbd6b52b4c32a8, 0xfff000001edb8786, FCR_UN },
  { 0x7ffbd6b52b4c32a8, 0xfff342ea1edb8786, FCR_UN },
  { 0x7ff00000bc88c2a9, 0xfff000002fa062f4, FCR_UN },
  { 0x7ff00000bc88c2a9, 0xfffdc9ee2fa062f4, FCR_UN },
  { 0x7ff8eaadbc88c2a9, 0xfff000002fa062f4, FCR_UN },
  { 0x7ff8eaadbc88c2a9, 0xfffdc9ee2fa062f4, FCR_UN },
  { 0x7ff00000a47525ca, 0x8000000000000000, FCR_UN },
  { 0x7ffcb028a47525ca, 0x8000000000000000, FCR_UN },
  { 0x7ff0000097c1af12, 0x8000000000000001, FCR_UN },
  { 0x7ffc541e97c1af12, 0x8000000000000001, FCR_UN },
  { 0x7ff00000bb1c07a4, 0x800fffffffffffff, FCR_UN },
  { 0x7ff966b7bb1c07a4, 0x800fffffffffffff, FCR_UN },
  { 0x7ff000001d98f07c, 0xbff0000000000000, FCR_UN },
  { 0x7ff9dbf61d98f07c, 0xbff0000000000000, FCR_UN },
  { 0x7ff0000040e65504, 0xffefffffffffffff, FCR_UN },
  { 0x7ffb2a7440e65504, 0xffefffffffffffff, FCR_UN },
  { 0x7ff00000d9dc7412, 0xfff0000000000000, FCR_UN },
  { 0x7ff8af62d9dc7412, 0xfff0000000000000, FCR_UN },
  { 0x8000000000000000, 0x0000000000000000, FCR_EQ },
  { 0x8000000000000000, 0x0000000000000001, FCR_LT },
  { 0x8000000000000000, 0x000fffffffffffff, FCR_LT },
  { 0x8000000000000000, 0x7fe0000000000000, FCR_LT },
  { 0x8000000000000000, 0x7ff0000000000000, FCR_LT },
  { 0x8000000000000000, 0x7ff000005a0faea3, FCR_UN },
  { 0x8000000000000000, 0x7ff225cc5a0faea3, FCR_UN },
  { 0x8000000000000000, 0x7ffa0cc436ad9daa, FCR_UN },
  { 0x8000000000000000, 0x8000000000000001, FCR_GT },
  { 0x8000000000000000, 0x800fffffffffffff, FCR_GT },
  { 0x8000000000000000, 0x8010000000000000, FCR_GT },
  { 0x8000000000000000, 0xbff0000000000000, FCR_GT },
  { 0x8000000000000000, 0xfff0000000000000, FCR_GT },
  { 0x8000000000000000, 0xfff000005a0faea3, FCR_UN },
  { 0x8000000000000000, 0xfff225cc5a0faea3, FCR_UN },
  { 0x8000000000000000, 0xfffa0cc436ad9daa, FCR_UN },
  { 0x8000000000000001, 0x0000000000000001, FCR_LT },
  { 0x8000000000000001, 0x3fefffffffffffff, FCR_LT },
  { 0x8000000000000001, 0x3ff0000000000000, FCR_LT },
  { 0x8000000000000001, 0x3ffffffffffffffe, FCR_LT },
  { 0x8000000000000001, 0x3fffffffffffffff, FCR_LT },
  { 0x8000000000000001, 0x7fdfffffffffffff, FCR_LT },
  { 0x8000000000000001, 0x7fe0000000000000, FCR_LT },
  { 0x8000000000000001, 0x7feffffffffffffe, FCR_LT },
  { 0x8000000000000001, 0x7fefffffffffffff, FCR_LT },
  { 0x8000000000000001, 0x7ff0000013fd5944, FCR_UN },
  { 0x8000000000000001, 0x7ff4154313fd5944, FCR_UN },
  { 0x8000000000000001, 0x7ffd397ba0f9b5e1, FCR_UN },
  { 0x8000000000000001, 0x8000000000000001, FCR_EQ },
  { 0x8000000000000001, 0xbfefffffffffffff, FCR_GT },
  { 0x8000000000000001, 0xbff0000000000000, FCR_GT },
  { 0x8000000000000001, 0xbffffffffffffffe, FCR_GT },
  { 0x8000000000000001, 0xbfffffffffffffff, FCR_GT },
  { 0x8000000000000001, 0xffdfffffffffffff, FCR_GT },
  { 0x8000000000000001, 0xffe0000000000000, FCR_GT },
  { 0x8000000000000001, 0xffeffffffffffffe, FCR_GT },
  { 0x8000000000000001, 0xffefffffffffffff, FCR_GT },
  { 0x8000000000000001, 0xfff0000013fd5944, FCR_UN },
  { 0x8000000000000001, 0xfff4154313fd5944, FCR_UN },
  { 0x8000000000000001, 0xfffd397ba0f9b5e1, FCR_UN },
  { 0x8000000000000002, 0x8000000000000001, FCR_LT },
  { 0x8000000000000003, 0x4008000000000000, FCR_LT },
  { 0x8000000000000003, 0x7fe0000000000000, FCR_LT },
  { 0x8000000000000003, 0x8000000000000002, FCR_LT },
  { 0x8000000000000003, 0xffe0000000000000, FCR_GT },
  { 0x8000000000000004, 0x8000000000000004, FCR_EQ },
  { 0x800ffffffffffffd, 0x800ffffffffffffe, FCR_GT },
  { 0x800fffffffffffff, 0x0000000000000000, FCR_LT },
  { 0x800fffffffffffff, 0x000fffffffffffff, FCR_LT },
  { 0x800fffffffffffff, 0x7ff0000000000000, FCR_LT },
  { 0x800fffffffffffff, 0x7ff00000a2b85efa, FCR_UN },
  { 0x800fffffffffffff, 0x7ff1d4fba2b85efa, FCR_UN },
  { 0x800fffffffffffff, 0x7ffd08c114a37fe6, FCR_UN },
  { 0x800fffffffffffff, 0x8000000000000000, FCR_LT },
  { 0x800fffffffffffff, 0x800ffffffffffffe, FCR_LT },
  { 0x800fffffffffffff, 0x800fffffffffffff, FCR_EQ },
  { 0x800fffffffffffff, 0x8010000000000000, FCR_GT },
  { 0x800fffffffffffff, 0xfff0000000000000, FCR_GT },
  { 0x800fffffffffffff, 0xfff00000a2b85efa, FCR_UN },
  { 0x800fffffffffffff, 0xfff1d4fba2b85efa, FCR_UN },
  { 0x800fffffffffffff, 0xfffd08c114a37fe6, FCR_UN },
  { 0x8010000000000000, 0x0000000000000000, FCR_LT },
  { 0x8010000000000000, 0x0010000000000000, FCR_LT },
  { 0x8010000000000001, 0x8010000000000000, FCR_LT },
  { 0x8010000000000001, 0x8010000000000002, FCR_GT },
  { 0x801fffffffffffff, 0x8020000000000000, FCR_GT },
  { 0x801fffffffffffff, 0x8020000000000002, FCR_GT },
  { 0x801fffffffffffff, 0x8020000000000004, FCR_GT },
  { 0x8020000000000000, 0x801fffffffffffff, FCR_LT },
  { 0x8020000000000001, 0x8010000000000001, FCR_LT },
  { 0x8020000000000001, 0x801fffffffffffff, FCR_LT },
  { 0x8020000000000002, 0x8010000000000001, FCR_LT },
  { 0x802fffffffffffff, 0x8030000000000000, FCR_GT },
  { 0x8030000000000000, 0x802fffffffffffff, FCR_LT },
  { 0x8030000000000001, 0x802fffffffffffff, FCR_LT },
  { 0x8030000000000002, 0x8020000000000003, FCR_LT },
  { 0xbff0000000000000, 0x3ff0000000000003, FCR_LT },
  { 0xbff0000000000000, 0x7ff000000d32ab76, FCR_UN },
  { 0xbff0000000000000, 0x7ff3d46c0d32ab76, FCR_UN },
  { 0xbff0000000000000, 0x7ffb51e7ffa1e86b, FCR_UN },
  { 0xbff0000000000000, 0x8000000000000000, FCR_LT },
  { 0xbff0000000000000, 0xbff0000000000003, FCR_GT },
  { 0xbff0000000000000, 0xfff000000d32ab76, FCR_UN },
  { 0xbff0000000000000, 0xfff3d46c0d32ab76, FCR_UN },
  { 0xbff0000000000000, 0xfffb51e7ffa1e86b, FCR_UN },
  { 0xbff0000000000001, 0x3ff0000000000000, FCR_LT },
  { 0xbff0000000000001, 0xbff0000000000000, FCR_LT },
  { 0xbff0000000000001, 0xbff0000000000002, FCR_GT },
  { 0xbffffffffffffffc, 0xbffffffffffffffd, FCR_GT },
  { 0xbfffffffffffffff, 0x0000000000000001, FCR_LT },
  { 0xbfffffffffffffff, 0xc000000000000000, FCR_GT },
  { 0xc000000000000000, 0x4000000000000001, FCR_LT },
  { 0xc000000000000000, 0xbfffffffffffffff, FCR_LT },
  { 0xc000000000000000, 0xc000000000000001, FCR_GT },
  { 0xc000000000000001, 0x4000000000000002, FCR_LT },
  { 0xc000000000000001, 0xbff0000000000001, FCR_LT },
  { 0xc000000000000001, 0xc000000000000002, FCR_GT },
  { 0xc000000000000002, 0xbff0000000000001, FCR_LT },
  { 0xc000000000000002, 0xbff0000000000003, FCR_LT },
  { 0xc000000000000004, 0xc000000000000003, FCR_LT },
  { 0xc008000000000000, 0x4008000000000000, FCR_LT },
  { 0xc00fffffffffffff, 0xc00ffffffffffffe, FCR_LT },
  { 0xc00fffffffffffff, 0xc010000000000002, FCR_GT },
  { 0xc010000000000001, 0xc00fffffffffffff, FCR_LT },
  { 0xffb0000000000001, 0xffafffffffffffff, FCR_LT },
  { 0xffcfffffffffffff, 0xffcffffffffffffe, FCR_LT },
  { 0xffcfffffffffffff, 0xffd0000000000002, FCR_GT },
  { 0xffd0000000000000, 0xffcfffffffffffff, FCR_LT },
  { 0xffd0000000000000, 0xffd0000000000001, FCR_GT },
  { 0xffd0000000000001, 0x7fd0000000000000, FCR_LT },
  { 0xffd0000000000001, 0xffd0000000000000, FCR_LT },
  { 0xffd0000000000001, 0xffe0000000000001, FCR_GT },
  { 0xffd0000000000002, 0xffc0000000000003, FCR_LT },
  { 0xffd0000000000004, 0xffd0000000000003, FCR_LT },
  { 0xffdffffffffffffe, 0x7fdffffffffffffe, FCR_LT },
  { 0xffdffffffffffffe, 0x7fdfffffffffffff, FCR_LT },
  { 0xffdffffffffffffe, 0xffdffffffffffffe, FCR_EQ },
  { 0xffdffffffffffffe, 0xffdfffffffffffff, FCR_GT },
  { 0xffdfffffffffffff, 0x3ff0000000000000, FCR_LT },
  { 0xffdfffffffffffff, 0x7fe0000000000000, FCR_LT },
  { 0xffdfffffffffffff, 0xbff0000000000000, FCR_LT },
  { 0xffdfffffffffffff, 0xffe0000000000000, FCR_GT },
  { 0xffe0000000000000, 0x0000000000000000, FCR_LT },
  { 0xffe0000000000000, 0x3ff0000000000000, FCR_LT },
  { 0xffe0000000000000, 0x7ff0000000000000, FCR_LT },
  { 0xffe0000000000000, 0x8000000000000000, FCR_LT },
  { 0xffe0000000000000, 0xbff0000000000000, FCR_LT },
  { 0xffe0000000000000, 0xffe0000000000000, FCR_EQ },
  { 0xffe0000000000000, 0xfff0000000000000, FCR_GT },
  { 0xffe0000000000001, 0x7fe0000000000000, FCR_LT },
  { 0xffe0000000000001, 0xffe0000000000000, FCR_LT },
  { 0xffe0000000000001, 0xffe0000000000002, FCR_GT },
  { 0xffe0000000000002, 0xffd0000000000001, FCR_LT },
  { 0xffeffffffffffffe, 0x3ff0000000000000, FCR_LT },
  { 0xffeffffffffffffe, 0x7fefffffffffffff, FCR_LT },
  { 0xffeffffffffffffe, 0xbff0000000000000, FCR_LT },
  { 0xffeffffffffffffe, 0xffefffffffffffff, FCR_GT },
  { 0xffefffffffffffff, 0x0000000000000001, FCR_LT },
  { 0xffefffffffffffff, 0x3ff0000000000000, FCR_LT },
  { 0xffefffffffffffff, 0x7ff000007d4a42a6, FCR_UN },
  { 0xffefffffffffffff, 0x7ff7252c7d4a42a6, FCR_UN },
  { 0xffefffffffffffff, 0x7ff980ec6115c6fb, FCR_UN },
  { 0xffefffffffffffff, 0x8000000000000001, FCR_LT },
  { 0xffefffffffffffff, 0xbff0000000000000, FCR_LT },
  { 0xffefffffffffffff, 0xffefffffffffffff, FCR_EQ },
  { 0xffefffffffffffff, 0xfff000007d4a42a6, FCR_UN },
  { 0xffefffffffffffff, 0xfff7252c7d4a42a6, FCR_UN },
  { 0xffefffffffffffff, 0xfff980ec6115c6fb, FCR_UN },
  { 0xfff0000000000000, 0x0000000000000000, FCR_LT },
  { 0xfff0000000000000, 0x0000000000000001, FCR_LT },
  { 0xfff0000000000000, 0x000fffffffffffff, FCR_LT },
  { 0xfff0000000000000, 0x7fe0000000000000, FCR_LT },
  { 0xfff0000000000000, 0x7fefffffffffffff, FCR_LT },
  { 0xfff0000000000000, 0x7ff0000000000000, FCR_LT },
  { 0xfff0000000000000, 0x7ff00000578bbe24, FCR_UN },
  { 0xfff0000000000000, 0x7ff63d54578bbe24, FCR_UN },
  { 0xfff0000000000000, 0x7ffbc66614390083, FCR_UN },
  { 0xfff0000000000000, 0x8000000000000000, FCR_LT },
  { 0xfff0000000000000, 0x8000000000000001, FCR_LT },
  { 0xfff0000000000000, 0x800fffffffffffff, FCR_LT },
  { 0xfff0000000000000, 0xffe0000000000000, FCR_LT },
  { 0xfff0000000000000, 0xffefffffffffffff, FCR_LT },
  { 0xfff0000000000000, 0xfff0000000000000, FCR_EQ },
  { 0xfff0000000000000, 0xfff00000578bbe24, FCR_UN },
  { 0xfff0000000000000, 0xfff63d54578bbe24, FCR_UN },
  { 0xfff0000000000000, 0xfffbc66614390083, FCR_UN },
  { 0xfff0000047e8b9a0, 0x0000000000000000, FCR_UN },
  { 0xfff4017647e8b9a0, 0x0000000000000000, FCR_UN },
  { 0xfff00000abfe5d29, 0x0000000000000001, FCR_UN },
  { 0xfff2a1cdabfe5d29, 0x0000000000000001, FCR_UN },
  { 0xfff000005155db76, 0x000fffffffffffff, FCR_UN },
  { 0xfff645cb5155db76, 0x000fffffffffffff, FCR_UN },
  { 0xfff0000070c46aa0, 0x3ff0000000000000, FCR_UN },
  { 0xfff2068470c46aa0, 0x3ff0000000000000, FCR_UN },
  { 0xfff00000b5aee637, 0x7fefffffffffffff, FCR_UN },
  { 0xfff72b19b5aee637, 0x7fefffffffffffff, FCR_UN },
  { 0xfff00000c08c2788, 0x7ff0000000000000, FCR_UN },
  { 0xfff1e0c1c08c2788, 0x7ff0000000000000, FCR_UN },
  { 0xfff00000ec581a54, 0x7ff0000021ebdfaf, FCR_UN },
  { 0xfff00000ec581a54, 0x7ff45d2221ebdfaf, FCR_UN },
  { 0xfff571eaec581a54, 0x7ff0000021ebdfaf, FCR_UN },
  { 0xfff571eaec581a54, 0x7ff45d2221ebdfaf, FCR_UN },
  { 0xfff000003a3a1f94, 0x7ff00000229f3502, FCR_UN },
  { 0xfff000003a3a1f94, 0x7ffb8fa0229f3502, FCR_UN },
  { 0xfff6439e3a3a1f94, 0x7ff00000229f3502, FCR_UN },
  { 0xfff6439e3a3a1f94, 0x7ffb8fa0229f3502, FCR_UN },
  { 0xfff00000ec581a54, 0xfff0000021ebdfaf, FCR_UN },
  { 0xfff00000ec581a54, 0xfff45d2221ebdfaf, FCR_UN },
  { 0xfff571eaec581a54, 0xfff0000021ebdfaf, FCR_UN },
  { 0xfff571eaec581a54, 0xfff45d2221ebdfaf, FCR_UN },
  { 0xfff000003a3a1f94, 0xfff00000229f3502, FCR_UN },
  { 0xfff000003a3a1f94, 0xfffb8fa0229f3502, FCR_UN },
  { 0xfff6439e3a3a1f94, 0xfff00000229f3502, FCR_UN },
  { 0xfff6439e3a3a1f94, 0xfffb8fa0229f3502, FCR_UN },
  { 0xfff00000c31d528e, 0x8000000000000000, FCR_UN },
  { 0xfff5fb72c31d528e, 0x8000000000000000, FCR_UN },
  { 0xfff00000ac81d215, 0x8000000000000001, FCR_UN },
  { 0xfff4481aac81d215, 0x8000000000000001, FCR_UN },
  { 0xfff00000d12062fd, 0x800fffffffffffff, FCR_UN },
  { 0xfff707f6d12062fd, 0x800fffffffffffff, FCR_UN },
  { 0xfff000001c6481ef, 0xbff0000000000000, FCR_UN },
  { 0xfff66ee91c6481ef, 0xbff0000000000000, FCR_UN },
  { 0xfff00000985729a7, 0xffefffffffffffff, FCR_UN },
  { 0xfff19cff985729a7, 0xffefffffffffffff, FCR_UN },
  { 0xfff0000053ec80fe, 0xfff0000000000000, FCR_UN },
  { 0xfff7dbc153ec80fe, 0xfff0000000000000, FCR_UN },
  { 0xfff00000816fb493, 0x0000000000000000, FCR_UN },
  { 0xfff87f75816fb493, 0x0000000000000000, FCR_UN },
  { 0xfff000000c2d7c33, 0x0000000000000001, FCR_UN },
  { 0xfff91ecb0c2d7c33, 0x0000000000000001, FCR_UN },
  { 0xfff00000a68bae40, 0x000fffffffffffff, FCR_UN },
  { 0xfffc0acda68bae40, 0x000fffffffffffff, FCR_UN },
  { 0xfff000002fe14961, 0x3ff0000000000000, FCR_UN },
  { 0xfffcfa4e2fe14961, 0x3ff0000000000000, FCR_UN },
  { 0xfff000005c206da1, 0x7fefffffffffffff, FCR_UN },
  { 0xfff800bb5c206da1, 0x7fefffffffffffff, FCR_UN },
  { 0xfff0000051887a34, 0x7ff0000000000000, FCR_UN },
  { 0xfffce11951887a34, 0x7ff0000000000000, FCR_UN },
  { 0xfff000002b4c32a8, 0x7ff000001edb8786, FCR_UN },
  { 0xfff000002b4c32a8, 0x7ff342ea1edb8786, FCR_UN },
  { 0xfffbd6b52b4c32a8, 0x7ff000001edb8786, FCR_UN },
  { 0xfffbd6b52b4c32a8, 0x7ff342ea1edb8786, FCR_UN },
  { 0xfff00000bc88c2a9, 0x7ff000002fa062f4, FCR_UN },
  { 0xfff00000bc88c2a9, 0x7ffdc9ee2fa062f4, FCR_UN },
  { 0xfff8eaadbc88c2a9, 0x7ff000002fa062f4, FCR_UN },
  { 0xfff8eaadbc88c2a9, 0x7ffdc9ee2fa062f4, FCR_UN },
  { 0xfff000002b4c32a8, 0xfff000001edb8786, FCR_UN },
  { 0xfff000002b4c32a8, 0xfff342ea1edb8786, FCR_UN },
  { 0xfffbd6b52b4c32a8, 0xfff000001edb8786, FCR_UN },
  { 0xfffbd6b52b4c32a8, 0xfff342ea1edb8786, FCR_UN },
  { 0xfff00000bc88c2a9, 0xfff000002fa062f4, FCR_UN },
  { 0xfff00000bc88c2a9, 0xfffdc9ee2fa062f4, FCR_UN },
  { 0xfff8eaadbc88c2a9, 0xfff000002fa062f4, FCR_UN },
  { 0xfff8eaadbc88c2a9, 0xfffdc9ee2fa062f4, FCR_UN },
  { 0xfff00000a47525ca, 0x8000000000000000, FCR_UN },
  { 0xfffcb028a47525ca, 0x8000000000000000, FCR_UN },
  { 0xfff0000097c1af12, 0x8000000000000001, FCR_UN },
  { 0xfffc541e97c1af12, 0x8000000000000001, FCR_UN },
  { 0xfff00000bb1c07a4, 0x800fffffffffffff, FCR_UN },
  { 0xfff966b7bb1c07a4, 0x800fffffffffffff, FCR_UN },
  { 0xfff000001d98f07c, 0xbff0000000000000, FCR_UN },
  { 0xfff9dbf61d98f07c, 0xbff0000000000000, FCR_UN },
  { 0xfff0000040e65504, 0xffefffffffffffff, FCR_UN },
  { 0xfffb2a7440e65504, 0xffefffffffffffff, FCR_UN },
  { 0xfff00000d9dc7412, 0xfff0000000000000, FCR_UN },
  { 0xfff8af62d9dc7412, 0xfff0000000000000, FCR_UN },
};

double
make_double (uint64_t x)
{
  double r;
  memcpy (&r, &x, sizeof (r));
  return r;
}

/* Three-way return value in the flags (plus a fourth state that
 * should never happen) */

enum Flag3 {
  FLAG3_LO = 0,
  FLAG3_EQ = 1,
  FLAG3_HI = 2,
  FLAG3_CONFUSED = 3,
};

static const char *const flag3_strings[] = {
  "LO", "EQ", "HI", "confused (C=0 but Z=1)",
};

#define CALL_FLAG3_RETURNING_FUNCTION(outvar, in0, in1, fn) do { \
    uint64_t operands[2] = { in0, in1 };                        \
    __asm__("push {r4,r5}\n\t"                                  \
            "movs r4, %1\n\t"                                   \
            "ldm r4!, {r0-r3}\n\t"                              \
            "pop {r4,r5}\n\t"                                   \
            "bl " fn "\n\t"                                     \
            "bhi 1f \n\t"                                       \
            "bcs 2f \n\t"                                       \
            "bne 3f \n\t"                                       \
            "movs %0, #3 \n\t"                                  \
            "b 4f \n\t"                                         \
            "1: movs %0, #2 \n\t"                               \
            "b 4f \n\t"                                         \
            "2: movs %0, #1 \n\t"                               \
            "b 4f \n\t"                                         \
            "3: movs %0, #0 \n\t"                               \
            "4:"                                                \
            : "=r" (outvar)                                     \
            : "r" (operands)                                    \
            : "r0", "r1", "r2", "r3", "r12", "r14", "cc", "memory");    \
      } while (0)

/* Two-way return value in the flags */

enum Flag2 {
  FLAG2_NE = 0,
  FLAG2_EQ = 1,
};

static const char *const flag2_strings[] = {
  "NE", "EQ",
};

#define CALL_FLAG2_RETURNING_FUNCTION(outvar, in0, in1, fn) do { \
    uint64_t operands[2] = { in0, in1 };                        \
    __asm__("push {r4,r5}\n\t"                                  \
            "movs r4, %1\n\t"                                   \
            "ldm r4!, {r0-r3}\n\t"                              \
            "pop {r4,r5}\n\t"                                   \
            "bl " fn "\n\t"                                     \
            "beq 1f \n\t"                                       \
            "movs %0, #0 \n\t"                                  \
            "b 2f \n\t"                                         \
            "1: movs %0, #1 \n\t"                               \
            "2:"                                                \
            : "=r" (outvar)                                     \
            : "r" (operands)                                    \
            : "r0", "r1", "r2", "r3", "r12", "r14", "cc", "memory");    \
      } while (0)

int
main (void)
{
  bool failed = false;

  for (size_t i = 0; i < sizeof (tests) / sizeof (tests[0]); i++)
    {
      const struct test *t = &tests[i];

#ifdef USE_NATIVE_ARITHMETIC
      /* If you compile with USE_NATIVE_ARITHMETIC defined, the same
       * set of tests will be run using the toolchain's built in float
       * arithmetic, instead of calling arm_fp_dcmp_*. */
      double in1 = make_double (t->in1);
      double in2 = make_double (t->in2);
#endif

      /* Test the individual comparison functions one at a time. */
      {
        extern uint32_t arm_fp_dcmp_bool_eq(uint64_t, uint64_t);
        unsigned eq_expected = (t->out == FCR_EQ);
#ifdef USE_NATIVE_ARITHMETIC
        unsigned eq_got = (in1 == in2);
#else
        unsigned eq_got = arm_fp_dcmp_bool_eq(t->in1, t->in2);
#endif

        if (eq_got != eq_expected)
	{
	  printf ("FAIL: dcmp_bool_eq(%016" PRIx64 ", %016" PRIx64 ") -> %u, expected %u (full result is '%s')\n",
		  t->in1, t->in2, eq_got, eq_expected, result_strings[t->out]);
	  failed = true;
	}
      }

      {
        extern uint32_t arm_fp_dcmp_bool_lt(uint64_t, uint64_t);
        unsigned lt_expected = (t->out == FCR_LT);
#ifdef USE_NATIVE_ARITHMETIC
        unsigned lt_got = isless(in1, in2);
#else
        unsigned lt_got = arm_fp_dcmp_bool_lt(t->in1, t->in2);
#endif

        if (lt_got != lt_expected)
	{
	  printf ("FAIL: dcmp_bool_lt(%016" PRIx64 ", %016" PRIx64 ") -> %u, expected %u (full result is '%s')\n",
		  t->in1, t->in2, lt_got, lt_expected, result_strings[t->out]);
	  failed = true;
	}
      }

      {
        extern uint32_t arm_fp_dcmp_bool_le(uint64_t, uint64_t);
        unsigned le_expected = (t->out == FCR_LT || t->out == FCR_EQ);
#ifdef USE_NATIVE_ARITHMETIC
        unsigned le_got = islessequal(in1, in2);
#else
        unsigned le_got = arm_fp_dcmp_bool_le(t->in1, t->in2);
#endif

        if (le_got != le_expected)
	{
	  printf ("FAIL: dcmp_bool_le(%016" PRIx64 ", %016" PRIx64 ") -> %u, expected %u (full result is '%s')\n",
		  t->in1, t->in2, le_got, le_expected, result_strings[t->out]);
	  failed = true;
	}
      }

      {
        extern uint32_t arm_fp_dcmp_bool_gt(uint64_t, uint64_t);
        unsigned gt_expected = (t->out == FCR_GT);
#ifdef USE_NATIVE_ARITHMETIC
        unsigned gt_got = isgreater(in1, in2);
#else
        unsigned gt_got = arm_fp_dcmp_bool_gt(t->in1, t->in2);
#endif

        if (gt_got != gt_expected)
	{
	  printf ("FAIL: dcmp_bool_gt(%016" PRIx64 ", %016" PRIx64 ") -> %u, expected %u (full result is '%s')\n",
		  t->in1, t->in2, gt_got, gt_expected, result_strings[t->out]);
	  failed = true;
	}
      }

      {
        extern uint32_t arm_fp_dcmp_bool_ge(uint64_t, uint64_t);
        unsigned ge_expected = (t->out == FCR_GT || t->out == FCR_EQ);
#ifdef USE_NATIVE_ARITHMETIC
        unsigned ge_got = isgreaterequal(in1, in2);
#else
        unsigned ge_got = arm_fp_dcmp_bool_ge(t->in1, t->in2);
#endif

        if (ge_got != ge_expected)
	{
	  printf ("FAIL: dcmp_bool_ge(%016" PRIx64 ", %016" PRIx64 ") -> %u, expected %u (full result is '%s')\n",
		  t->in1, t->in2, ge_got, ge_expected, result_strings[t->out]);
	  failed = true;
	}
      }

      {
        extern uint32_t arm_fp_dcmp_bool_un(uint64_t, uint64_t);
        unsigned un_expected = (t->out == FCR_UN);
#ifdef USE_NATIVE_ARITHMETIC
        unsigned un_got = isunordered(in1, in2);
#else
        unsigned un_got = arm_fp_dcmp_bool_un(t->in1, t->in2);
#endif

        if (un_got != un_expected)
	{
	  printf ("FAIL: dcmp_bool_un(%016" PRIx64 ", %016" PRIx64 ") -> %u, expected %u (full result is '%s')\n",
		  t->in1, t->in2, un_got, un_expected, result_strings[t->out]);
	  failed = true;
	}
      }

      {
        unsigned fl_expected = (t->out == FCR_EQ ? FLAG2_EQ : FLAG2_NE);
        unsigned fl_got;
        CALL_FLAG2_RETURNING_FUNCTION(fl_got, t->in1, t->in2, "arm_fp_dcmp_flags_eq");

        if (fl_got != fl_expected)
	{
	  printf ("FAIL: dcmp_flags_eq(%016" PRIx64 ", %016" PRIx64 ") -> %s, expected %s (full result is '%s')\n",
		  t->in1, t->in2, flag2_strings[fl_got], flag2_strings[fl_expected], result_strings[t->out]);
	  failed = true;
	}
      }

      {
        unsigned fl_expected = (t->out == FCR_EQ ? FLAG3_EQ :
                                t->out == FCR_LT ? FLAG3_LO :
                                FLAG3_HI);
        unsigned fl_got;
        CALL_FLAG3_RETURNING_FUNCTION(fl_got, t->in1, t->in2, "arm_fp_dcmp_flags");

        if (fl_got != fl_expected)
	{
	  printf ("FAIL: dcmp_flags(%016" PRIx64 ", %016" PRIx64 ") -> %s, expected %s (full result is '%s')\n",
		  t->in1, t->in2, flag3_strings[fl_got], flag3_strings[fl_expected], result_strings[t->out]);
	  failed = true;
	}
      }

      {
        unsigned fl_expected = (t->out == FCR_EQ ? FLAG3_EQ :
                                t->out == FCR_GT ? FLAG3_LO :
                                FLAG3_HI);
        unsigned fl_got;
        CALL_FLAG3_RETURNING_FUNCTION(fl_got, t->in1, t->in2, "arm_fp_dcmp_flags_rev");

        if (fl_got != fl_expected)
	{
	  printf ("FAIL: dcmp_flags_rev(%016" PRIx64 ", %016" PRIx64 ") -> %s, expected %s (full result is '%s')\n",
		  t->in1, t->in2, flag3_strings[fl_got], flag3_strings[fl_expected], result_strings[t->out]);
	  failed = true;
	}
      }
    }

  if (!failed)
    printf ("all passed\n");

  return failed;
}
