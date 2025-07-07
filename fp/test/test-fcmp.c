/*
 * Tests of IEEE 754 single-precision division
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
  uint32_t in1, in2;
  enum FloatCompareResult out;
};

static const struct test tests[] = {
  { 0x00000000, 0x00000001, FCR_LT },
  { 0x00000000, 0x007fffff, FCR_LT },
  { 0x00000000, 0x3f800000, FCR_LT },
  { 0x00000000, 0x7f000000, FCR_LT },
  { 0x00000000, 0x7f800000, FCR_LT },
  { 0x00000000, 0x7f872da0, FCR_UN },
  { 0x00000000, 0x7fe42e09, FCR_UN },
  { 0x00000000, 0x80000000, FCR_EQ },
  { 0x00000000, 0x80000001, FCR_GT },
  { 0x00000000, 0x807fffff, FCR_GT },
  { 0x00000000, 0x80800000, FCR_GT },
  { 0x00000000, 0xff800000, FCR_GT },
  { 0x00000001, 0x00000001, FCR_EQ },
  { 0x00000001, 0x3f7fffff, FCR_LT },
  { 0x00000001, 0x3f800000, FCR_LT },
  { 0x00000001, 0x3ffffffe, FCR_LT },
  { 0x00000001, 0x3fffffff, FCR_LT },
  { 0x00000001, 0x7effffff, FCR_LT },
  { 0x00000001, 0x7f000000, FCR_LT },
  { 0x00000001, 0x7f7ffffe, FCR_LT },
  { 0x00000001, 0x7f7fffff, FCR_LT },
  { 0x00000001, 0x7f94d5b9, FCR_UN },
  { 0x00000001, 0x7fef53b1, FCR_UN },
  { 0x00000001, 0x80000001, FCR_GT },
  { 0x00000001, 0xbf7fffff, FCR_GT },
  { 0x00000001, 0xbf800000, FCR_GT },
  { 0x00000001, 0xbffffffe, FCR_GT },
  { 0x00000001, 0xbfffffff, FCR_GT },
  { 0x00000001, 0xfeffffff, FCR_GT },
  { 0x00000001, 0xff000000, FCR_GT },
  { 0x00000001, 0xff7ffffe, FCR_GT },
  { 0x00000001, 0xff7fffff, FCR_GT },
  { 0x00000002, 0x00000001, FCR_GT },
  { 0x00000003, 0x00000002, FCR_GT },
  { 0x00000003, 0x40400000, FCR_LT },
  { 0x00000003, 0x40a00000, FCR_LT },
  { 0x00000003, 0x7f000000, FCR_LT },
  { 0x00000003, 0xc0a00000, FCR_GT },
  { 0x00000003, 0xff000000, FCR_GT },
  { 0x00000004, 0x00000004, FCR_EQ },
  { 0x007ffffc, 0x807ffffc, FCR_GT },
  { 0x007ffffd, 0x007ffffe, FCR_LT },
  { 0x007fffff, 0x00000000, FCR_GT },
  { 0x007fffff, 0x007ffffe, FCR_GT },
  { 0x007fffff, 0x007fffff, FCR_EQ },
  { 0x007fffff, 0x00800000, FCR_LT },
  { 0x007fffff, 0x7f800000, FCR_LT },
  { 0x007fffff, 0x7fa111d3, FCR_UN },
  { 0x007fffff, 0x7ff43134, FCR_UN },
  { 0x007fffff, 0x80000000, FCR_GT },
  { 0x007fffff, 0xff800000, FCR_GT },
  { 0x00800000, 0x00000000, FCR_GT },
  { 0x00800000, 0x00800000, FCR_EQ },
  { 0x00800000, 0x80800000, FCR_GT },
  { 0x00800001, 0x00800000, FCR_GT },
  { 0x00800001, 0x00800002, FCR_LT },
  { 0x00ffffff, 0x01000000, FCR_LT },
  { 0x00ffffff, 0x01000002, FCR_LT },
  { 0x00ffffff, 0x01000004, FCR_LT },
  { 0x01000000, 0x00ffffff, FCR_GT },
  { 0x01000001, 0x00800001, FCR_GT },
  { 0x01000001, 0x00ffffff, FCR_GT },
  { 0x01000002, 0x00800001, FCR_GT },
  { 0x017fffff, 0x01800000, FCR_LT },
  { 0x01800000, 0x017fffff, FCR_GT },
  { 0x01800001, 0x017fffff, FCR_GT },
  { 0x01800002, 0x01000003, FCR_GT },
  { 0x3f000000, 0x3f000000, FCR_EQ },
  { 0x3f7fffff, 0x00000001, FCR_GT },
  { 0x3f7fffff, 0x80000001, FCR_GT },
  { 0x3f800000, 0x3f800000, FCR_EQ },
  { 0x3f800000, 0x3f800003, FCR_LT },
  { 0x3f800000, 0x40000000, FCR_LT },
  { 0x3f800000, 0x40e00000, FCR_LT },
  { 0x3f800000, 0x7fb27f62, FCR_UN },
  { 0x3f800000, 0x7fd9d4b4, FCR_UN },
  { 0x3f800000, 0x80000000, FCR_GT },
  { 0x3f800000, 0xbf800000, FCR_GT },
  { 0x3f800000, 0xbf800003, FCR_GT },
  { 0x3f800001, 0x3f800000, FCR_GT },
  { 0x3f800001, 0x3f800002, FCR_LT },
  { 0x3f800001, 0xbf800000, FCR_GT },
  { 0x3ffffffc, 0x3ffffffd, FCR_LT },
  { 0x3fffffff, 0x00000001, FCR_GT },
  { 0x3fffffff, 0x40000000, FCR_LT },
  { 0x40000000, 0x3f800000, FCR_GT },
  { 0x40000000, 0x3fffffff, FCR_GT },
  { 0x40000000, 0x40000000, FCR_EQ },
  { 0x40000000, 0x40000001, FCR_LT },
  { 0x40000000, 0xc0000000, FCR_GT },
  { 0x40000000, 0xc0000001, FCR_GT },
  { 0x40000000, 0xc0a00000, FCR_GT },
  { 0x40000001, 0x3f800001, FCR_GT },
  { 0x40000001, 0x40000002, FCR_LT },
  { 0x40000001, 0xc0000002, FCR_GT },
  { 0x40000002, 0x3f800001, FCR_GT },
  { 0x40000002, 0x3f800003, FCR_GT },
  { 0x40000004, 0x40000003, FCR_GT },
  { 0x40400000, 0x40400000, FCR_EQ },
  { 0x407fffff, 0x407ffffe, FCR_GT },
  { 0x407fffff, 0x40800002, FCR_LT },
  { 0x40800001, 0x407fffff, FCR_GT },
  { 0x40a00000, 0x00000000, FCR_GT },
  { 0x40a00000, 0x80000000, FCR_GT },
  { 0x40a00000, 0xbf800000, FCR_GT },
  { 0x40a00000, 0xc0a00000, FCR_GT },
  { 0x7d800001, 0x7d7fffff, FCR_GT },
  { 0x7e7fffff, 0x7e7ffffe, FCR_GT },
  { 0x7e7fffff, 0x7e800002, FCR_LT },
  { 0x7e800000, 0x7e7fffff, FCR_GT },
  { 0x7e800000, 0x7e800000, FCR_EQ },
  { 0x7e800000, 0x7e800001, FCR_LT },
  { 0x7e800001, 0x7e800000, FCR_GT },
  { 0x7e800001, 0x7f000001, FCR_LT },
  { 0x7e800001, 0xfe800000, FCR_GT },
  { 0x7e800002, 0x7e000003, FCR_GT },
  { 0x7e800004, 0x7e800003, FCR_GT },
  { 0x7efffffe, 0x7efffffe, FCR_EQ },
  { 0x7efffffe, 0x7effffff, FCR_LT },
  { 0x7efffffe, 0xfeffffff, FCR_GT },
  { 0x7effffff, 0x3f800000, FCR_GT },
  { 0x7effffff, 0x7f000000, FCR_LT },
  { 0x7effffff, 0xbf800000, FCR_GT },
  { 0x7effffff, 0xff000000, FCR_GT },
  { 0x7f000000, 0x3f800000, FCR_GT },
  { 0x7f000000, 0x7f000000, FCR_EQ },
  { 0x7f000000, 0x7f800000, FCR_LT },
  { 0x7f000000, 0xbf800000, FCR_GT },
  { 0x7f000000, 0xff000000, FCR_GT },
  { 0x7f000000, 0xff800000, FCR_GT },
  { 0x7f000001, 0x7f000000, FCR_GT },
  { 0x7f000001, 0x7f000002, FCR_LT },
  { 0x7f000001, 0xff000000, FCR_GT },
  { 0x7f000002, 0x7e800001, FCR_GT },
  { 0x7f7ffffe, 0x3f800000, FCR_GT },
  { 0x7f7ffffe, 0x7f7fffff, FCR_LT },
  { 0x7f7ffffe, 0xbf800000, FCR_GT },
  { 0x7f7ffffe, 0xff7fffff, FCR_GT },
  { 0x7f7fffff, 0x00000001, FCR_GT },
  { 0x7f7fffff, 0x3f800000, FCR_GT },
  { 0x7f7fffff, 0x7f7fffff, FCR_EQ },
  { 0x7f7fffff, 0x7fbed1eb, FCR_UN },
  { 0x7f7fffff, 0x7fe15ee3, FCR_UN },
  { 0x7f7fffff, 0x80000001, FCR_GT },
  { 0x7f7fffff, 0xbf800000, FCR_GT },
  { 0x7f800000, 0x00000000, FCR_GT },
  { 0x7f800000, 0x00000001, FCR_GT },
  { 0x7f800000, 0x007fffff, FCR_GT },
  { 0x7f800000, 0x7f000000, FCR_GT },
  { 0x7f800000, 0x7f7fffff, FCR_GT },
  { 0x7f800000, 0x7f800000, FCR_EQ },
  { 0x7f800000, 0x7f91a4da, FCR_UN },
  { 0x7f800000, 0x7fd44a09, FCR_UN },
  { 0x7f800000, 0x80000000, FCR_GT },
  { 0x7f800000, 0x80000001, FCR_GT },
  { 0x7f800000, 0x807fffff, FCR_GT },
  { 0x7f800000, 0xff000000, FCR_GT },
  { 0x7f800000, 0xff7fffff, FCR_GT },
  { 0x7f800000, 0xff800000, FCR_GT },
  { 0x7f86d066, 0x00000000, FCR_UN },
  { 0x7f85a878, 0x00000001, FCR_UN },
  { 0x7f8c0dca, 0x007fffff, FCR_UN },
  { 0x7f822725, 0x3f800000, FCR_UN },
  { 0x7f853870, 0x7f7fffff, FCR_UN },
  { 0x7fbefc9d, 0x7f800000, FCR_UN },
  { 0x7f9f84a9, 0x7f81461b, FCR_UN },
  { 0x7f9e2c1d, 0x7fe4a313, FCR_UN },
  { 0x7fb0e6d0, 0x80000000, FCR_UN },
  { 0x7fac9171, 0x80000001, FCR_UN },
  { 0x7f824ae6, 0x807fffff, FCR_UN },
  { 0x7fa8b9a0, 0xbf800000, FCR_UN },
  { 0x7f92a1cd, 0xff7fffff, FCR_UN },
  { 0x7fbe5d29, 0xff800000, FCR_UN },
  { 0x7fcc9a57, 0x00000000, FCR_UN },
  { 0x7fec9d71, 0x00000001, FCR_UN },
  { 0x7fd5db76, 0x007fffff, FCR_UN },
  { 0x7fd003d9, 0x3f800000, FCR_UN },
  { 0x7fca0684, 0x7f7fffff, FCR_UN },
  { 0x7fc46aa0, 0x7f800000, FCR_UN },
  { 0x7ff72b19, 0x7faee637, FCR_UN },
  { 0x7fe9e0c1, 0x7fcc2788, FCR_UN },
  { 0x7fc571ea, 0x80000000, FCR_UN },
  { 0x7fd81a54, 0x80000001, FCR_UN },
  { 0x7febdfaf, 0x807fffff, FCR_UN },
  { 0x7ffa1f94, 0xbf800000, FCR_UN },
  { 0x7ff38fa0, 0xff7fffff, FCR_UN },
  { 0x7fdf3502, 0xff800000, FCR_UN },
  { 0x80000000, 0x00000000, FCR_EQ },
  { 0x80000000, 0x00000001, FCR_LT },
  { 0x80000000, 0x007fffff, FCR_LT },
  { 0x80000000, 0x7f000000, FCR_LT },
  { 0x80000000, 0x7f800000, FCR_LT },
  { 0x80000000, 0x7fbdfb72, FCR_UN },
  { 0x80000000, 0x7fdd528e, FCR_UN },
  { 0x80000000, 0x80000001, FCR_GT },
  { 0x80000000, 0x807fffff, FCR_GT },
  { 0x80000000, 0x80800000, FCR_GT },
  { 0x80000000, 0xbf800000, FCR_GT },
  { 0x80000000, 0xff800000, FCR_GT },
  { 0x80000001, 0x00000001, FCR_LT },
  { 0x80000001, 0x3f7fffff, FCR_LT },
  { 0x80000001, 0x3f800000, FCR_LT },
  { 0x80000001, 0x3ffffffe, FCR_LT },
  { 0x80000001, 0x3fffffff, FCR_LT },
  { 0x80000001, 0x7effffff, FCR_LT },
  { 0x80000001, 0x7f000000, FCR_LT },
  { 0x80000001, 0x7f7ffffe, FCR_LT },
  { 0x80000001, 0x7f7fffff, FCR_LT },
  { 0x80000001, 0x7fac481a, FCR_UN },
  { 0x80000001, 0x7fcf111d, FCR_UN },
  { 0x80000001, 0x80000001, FCR_EQ },
  { 0x80000001, 0xbf7fffff, FCR_GT },
  { 0x80000001, 0xbf800000, FCR_GT },
  { 0x80000001, 0xbffffffe, FCR_GT },
  { 0x80000001, 0xbfffffff, FCR_GT },
  { 0x80000001, 0xfeffffff, FCR_GT },
  { 0x80000001, 0xff000000, FCR_GT },
  { 0x80000001, 0xff7ffffe, FCR_GT },
  { 0x80000001, 0xff7fffff, FCR_GT },
  { 0x80000002, 0x80000001, FCR_LT },
  { 0x80000003, 0x40400000, FCR_LT },
  { 0x80000003, 0x7f000000, FCR_LT },
  { 0x80000003, 0x80000002, FCR_LT },
  { 0x80000003, 0xff000000, FCR_GT },
  { 0x80000004, 0x80000004, FCR_EQ },
  { 0x807ffffd, 0x807ffffe, FCR_GT },
  { 0x807fffff, 0x00000000, FCR_LT },
  { 0x807fffff, 0x007fffff, FCR_LT },
  { 0x807fffff, 0x7f800000, FCR_LT },
  { 0x807fffff, 0x7faf07f6, FCR_UN },
  { 0x807fffff, 0x7fd18a54, FCR_UN },
  { 0x807fffff, 0x80000000, FCR_LT },
  { 0x807fffff, 0x807ffffe, FCR_LT },
  { 0x807fffff, 0x807fffff, FCR_EQ },
  { 0x807fffff, 0x80800000, FCR_GT },
  { 0x807fffff, 0xff800000, FCR_GT },
  { 0x80800000, 0x00000000, FCR_LT },
  { 0x80800000, 0x00800000, FCR_LT },
  { 0x80800001, 0x80800000, FCR_LT },
  { 0x80800001, 0x80800002, FCR_GT },
  { 0x80ffffff, 0x81000000, FCR_GT },
  { 0x80ffffff, 0x81000002, FCR_GT },
  { 0x80ffffff, 0x81000004, FCR_GT },
  { 0x81000000, 0x80ffffff, FCR_LT },
  { 0x81000001, 0x80800001, FCR_LT },
  { 0x81000001, 0x80ffffff, FCR_LT },
  { 0x81000002, 0x80800001, FCR_LT },
  { 0x817fffff, 0x81800000, FCR_GT },
  { 0x81800000, 0x817fffff, FCR_LT },
  { 0x81800001, 0x817fffff, FCR_LT },
  { 0x81800002, 0x81000003, FCR_LT },
  { 0xbf800000, 0x3f800003, FCR_LT },
  { 0xbf800000, 0x7fa66ee9, FCR_UN },
  { 0xbf800000, 0x7fe481ef, FCR_UN },
  { 0xbf800000, 0x80000000, FCR_LT },
  { 0xbf800000, 0xbf800003, FCR_GT },
  { 0xbf800001, 0x3f800000, FCR_LT },
  { 0xbf800001, 0xbf800000, FCR_LT },
  { 0xbf800001, 0xbf800002, FCR_GT },
  { 0xbffffffc, 0xbffffffd, FCR_GT },
  { 0xbfffffff, 0x00000001, FCR_LT },
  { 0xbfffffff, 0xc0000000, FCR_GT },
  { 0xc0000000, 0x40000001, FCR_LT },
  { 0xc0000000, 0xbfffffff, FCR_LT },
  { 0xc0000000, 0xc0000001, FCR_GT },
  { 0xc0000001, 0x40000002, FCR_LT },
  { 0xc0000001, 0xbf800001, FCR_LT },
  { 0xc0000001, 0xc0000002, FCR_GT },
  { 0xc0000002, 0xbf800001, FCR_LT },
  { 0xc0000002, 0xbf800003, FCR_LT },
  { 0xc0000004, 0xc0000003, FCR_LT },
  { 0xc0400000, 0x40400000, FCR_LT },
  { 0xc07fffff, 0xc07ffffe, FCR_LT },
  { 0xc07fffff, 0xc0800002, FCR_GT },
  { 0xc0800001, 0xc07fffff, FCR_LT },
  { 0xfd800001, 0xfd7fffff, FCR_LT },
  { 0xfe7fffff, 0xfe7ffffe, FCR_LT },
  { 0xfe7fffff, 0xfe800002, FCR_GT },
  { 0xfe800000, 0xfe7fffff, FCR_LT },
  { 0xfe800000, 0xfe800001, FCR_GT },
  { 0xfe800001, 0x7e800000, FCR_LT },
  { 0xfe800001, 0xfe800000, FCR_LT },
  { 0xfe800001, 0xff000001, FCR_GT },
  { 0xfe800002, 0xfe000003, FCR_LT },
  { 0xfe800004, 0xfe800003, FCR_LT },
  { 0xfefffffe, 0x7efffffe, FCR_LT },
  { 0xfefffffe, 0x7effffff, FCR_LT },
  { 0xfefffffe, 0xfefffffe, FCR_EQ },
  { 0xfefffffe, 0xfeffffff, FCR_GT },
  { 0xfeffffff, 0x3f800000, FCR_LT },
  { 0xfeffffff, 0x7f000000, FCR_LT },
  { 0xfeffffff, 0xbf800000, FCR_LT },
  { 0xfeffffff, 0xff000000, FCR_GT },
  { 0xff000000, 0x00000000, FCR_LT },
  { 0xff000000, 0x3f800000, FCR_LT },
  { 0xff000000, 0x7f800000, FCR_LT },
  { 0xff000000, 0x80000000, FCR_LT },
  { 0xff000000, 0xbf800000, FCR_LT },
  { 0xff000000, 0xff000000, FCR_EQ },
  { 0xff000000, 0xff800000, FCR_GT },
  { 0xff000001, 0x7f000000, FCR_LT },
  { 0xff000001, 0xff000000, FCR_LT },
  { 0xff000001, 0xff000002, FCR_GT },
  { 0xff000002, 0xfe800001, FCR_LT },
  { 0xff7ffffe, 0x3f800000, FCR_LT },
  { 0xff7ffffe, 0x7f7fffff, FCR_LT },
  { 0xff7ffffe, 0xbf800000, FCR_LT },
  { 0xff7ffffe, 0xff7fffff, FCR_GT },
  { 0xff7fffff, 0x00000001, FCR_LT },
  { 0xff7fffff, 0x3f800000, FCR_LT },
  { 0xff7fffff, 0x7f919cff, FCR_UN },
  { 0xff7fffff, 0x7fd729a7, FCR_UN },
  { 0xff7fffff, 0x80000001, FCR_LT },
  { 0xff7fffff, 0xbf800000, FCR_LT },
  { 0xff7fffff, 0xff7fffff, FCR_EQ },
  { 0xff800000, 0x00000000, FCR_LT },
  { 0xff800000, 0x00000001, FCR_LT },
  { 0xff800000, 0x007fffff, FCR_LT },
  { 0xff800000, 0x7f000000, FCR_LT },
  { 0xff800000, 0x7f7fffff, FCR_LT },
  { 0xff800000, 0x7f800000, FCR_LT },
  { 0xff800000, 0x7fafdbc1, FCR_UN },
  { 0xff800000, 0x7fec80fe, FCR_UN },
  { 0xff800000, 0x80000000, FCR_LT },
  { 0xff800000, 0x80000001, FCR_LT },
  { 0xff800000, 0x807fffff, FCR_LT },
  { 0xff800000, 0xff000000, FCR_LT },
  { 0xff800000, 0xff7fffff, FCR_LT },
  { 0xff800000, 0xff800000, FCR_EQ },
};

float
make_float (uint32_t x)
{
  float r;
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
    register uint32_t r0 __asm__("r0");                         \
    register uint32_t r1 __asm__("r1");                         \
    r0 = in0;                                                   \
    r1 = in1;                                                   \
    __asm__("bl " fn "\n\t"                                     \
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
            : "r" (r0), "r" (r1)                                \
            : "r2", "r3", "r12", "r14", "cc");                  \
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
    register uint32_t r0 __asm__("r0");                         \
    register uint32_t r1 __asm__("r1");                         \
    r0 = in0;                                                   \
    r1 = in1;                                                   \
    __asm__("bl " fn "\n\t"                                     \
            "beq 1f \n\t"                                       \
            "movs %0, #0 \n\t"                                  \
            "b 2f \n\t"                                         \
            "1: movs %0, #1 \n\t"                               \
            "2:"                                                \
            : "=r" (outvar)                                     \
            : "r" (r0), "r" (r1)                                \
            : "r2", "r3", "r12", "r14", "cc");                  \
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
       * arithmetic, instead of calling arm_fp_fcmp_*. */
      float in1 = make_float (t->in1);
      float in2 = make_float (t->in2);
#endif

      /* Test the individual comparison functions one at a time. */
      {
        extern uint32_t arm_fp_fcmp_bool_eq(uint32_t, uint32_t);
        unsigned eq_expected = (t->out == FCR_EQ);
#ifdef USE_NATIVE_ARITHMETIC
        unsigned eq_got = (in1 == in2);
#else
        unsigned eq_got = arm_fp_fcmp_bool_eq(t->in1, t->in2);
#endif

        if (eq_got != eq_expected)
	{
	  printf ("FAIL: fcmp_bool_eq(%08" PRIx32 ", %08" PRIx32 ") -> %u, expected %u (full result is '%s')\n",
		  t->in1, t->in2, eq_got, eq_expected, result_strings[t->out]);
	  failed = true;
	}
      }

      {
        extern uint32_t arm_fp_fcmp_bool_lt(uint32_t, uint32_t);
        unsigned lt_expected = (t->out == FCR_LT);
#ifdef USE_NATIVE_ARITHMETIC
        unsigned lt_got = isless(in1, in2);
#else
        unsigned lt_got = arm_fp_fcmp_bool_lt(t->in1, t->in2);
#endif

        if (lt_got != lt_expected)
	{
	  printf ("FAIL: fcmp_bool_lt(%08" PRIx32 ", %08" PRIx32 ") -> %u, expected %u (full result is '%s')\n",
		  t->in1, t->in2, lt_got, lt_expected, result_strings[t->out]);
	  failed = true;
	}
      }

      {
        extern uint32_t arm_fp_fcmp_bool_le(uint32_t, uint32_t);
        unsigned le_expected = (t->out == FCR_LT || t->out == FCR_EQ);
#ifdef USE_NATIVE_ARITHMETIC
        unsigned le_got = islessequal(in1, in2);
#else
        unsigned le_got = arm_fp_fcmp_bool_le(t->in1, t->in2);
#endif

        if (le_got != le_expected)
	{
	  printf ("FAIL: fcmp_bool_le(%08" PRIx32 ", %08" PRIx32 ") -> %u, expected %u (full result is '%s')\n",
		  t->in1, t->in2, le_got, le_expected, result_strings[t->out]);
	  failed = true;
	}
      }

      {
        extern uint32_t arm_fp_fcmp_bool_gt(uint32_t, uint32_t);
        unsigned gt_expected = (t->out == FCR_GT);
#ifdef USE_NATIVE_ARITHMETIC
        unsigned gt_got = isgreater(in1, in2);
#else
        unsigned gt_got = arm_fp_fcmp_bool_gt(t->in1, t->in2);
#endif

        if (gt_got != gt_expected)
	{
	  printf ("FAIL: fcmp_bool_gt(%08" PRIx32 ", %08" PRIx32 ") -> %u, expected %u (full result is '%s')\n",
		  t->in1, t->in2, gt_got, gt_expected, result_strings[t->out]);
	  failed = true;
	}
      }

      {
        extern uint32_t arm_fp_fcmp_bool_ge(uint32_t, uint32_t);
        unsigned ge_expected = (t->out == FCR_GT || t->out == FCR_EQ);
#ifdef USE_NATIVE_ARITHMETIC
        unsigned ge_got = isgreaterequal(in1, in2);
#else
        unsigned ge_got = arm_fp_fcmp_bool_ge(t->in1, t->in2);
#endif

        if (ge_got != ge_expected)
	{
	  printf ("FAIL: fcmp_bool_ge(%08" PRIx32 ", %08" PRIx32 ") -> %u, expected %u (full result is '%s')\n",
		  t->in1, t->in2, ge_got, ge_expected, result_strings[t->out]);
	  failed = true;
	}
      }

      {
        extern uint32_t arm_fp_fcmp_bool_un(uint32_t, uint32_t);
        unsigned un_expected = (t->out == FCR_UN);
#ifdef USE_NATIVE_ARITHMETIC
        unsigned un_got = isunordered(in1, in2);
#else
        unsigned un_got = arm_fp_fcmp_bool_un(t->in1, t->in2);
#endif

        if (un_got != un_expected)
	{
	  printf ("FAIL: fcmp_bool_un(%08" PRIx32 ", %08" PRIx32 ") -> %u, expected %u (full result is '%s')\n",
		  t->in1, t->in2, un_got, un_expected, result_strings[t->out]);
	  failed = true;
	}
      }

      {
        unsigned fl_expected = (t->out == FCR_EQ ? FLAG2_EQ : FLAG2_NE);
        unsigned fl_got;
        CALL_FLAG2_RETURNING_FUNCTION(fl_got, t->in1, t->in2, "arm_fp_fcmp_flags_eq");

        if (fl_got != fl_expected)
	{
	  printf ("FAIL: fcmp_flags_eq(%08" PRIx32 ", %08" PRIx32 ") -> %s, expected %s (full result is '%s')\n",
		  t->in1, t->in2, flag2_strings[fl_got], flag2_strings[fl_expected], result_strings[t->out]);
	  failed = true;
	}
      }

      {
        unsigned fl_expected = (t->out == FCR_EQ ? FLAG3_EQ :
                                t->out == FCR_LT ? FLAG3_LO :
                                FLAG3_HI);
        unsigned fl_got;
        CALL_FLAG3_RETURNING_FUNCTION(fl_got, t->in1, t->in2, "arm_fp_fcmp_flags");

        if (fl_got != fl_expected)
	{
	  printf ("FAIL: fcmp_flags(%08" PRIx32 ", %08" PRIx32 ") -> %s, expected %s (full result is '%s')\n",
		  t->in1, t->in2, flag3_strings[fl_got], flag3_strings[fl_expected], result_strings[t->out]);
	  failed = true;
	}
      }

      {
        unsigned fl_expected = (t->out == FCR_EQ ? FLAG3_EQ :
                                t->out == FCR_GT ? FLAG3_LO :
                                FLAG3_HI);
        unsigned fl_got;
        CALL_FLAG3_RETURNING_FUNCTION(fl_got, t->in1, t->in2, "arm_fp_fcmp_flags_rev");

        if (fl_got != fl_expected)
	{
	  printf ("FAIL: fcmp_flags_rev(%08" PRIx32 ", %08" PRIx32 ") -> %s, expected %s (full result is '%s')\n",
		  t->in1, t->in2, flag3_strings[fl_got], flag3_strings[fl_expected], result_strings[t->out]);
	  failed = true;
	}
      }
    }

  if (!failed)
    printf ("all passed\n");

  return failed;
}
