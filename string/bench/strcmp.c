/*
 * strcmp benchmark.
 *
 * Copyright (c) 2026, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#define _GNU_SOURCE
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "stringlib.h"
#include "benchlib.h"

#define ITERS_RANDOM	2000
#define ITERS_SMALL	20000000
#define ITERS_MEDIUM	1000000

#define NUM_TESTS 65536

#define MAX_ALIGN 32
#define MAX_STRLEN 128

/* Offset of the second operand in the mutually misaligned benchmarks.  It is
   not a multiple of 8 so that the two operands cannot share a load offset,
   which is what selects the unaligned main loop in strcmp.S.  */
#define MISALIGN 3

/* The buffers are page aligned and equally sized, so without a skew the two
   operands of a comparison sit an exact power of two apart.  That alone made
   the mutually misaligned benchmark come out faster than the mutually aligned
   one, which is backwards: measured as the ratio of the two sections within a
   single run, the skew is worth 61% on Cortex-A510 and 11% on Cortex-X3.  The
   skews are multiples of 8 so the operands stay mutually aligned and the
   aligned main loop is still the one being measured.  */
#define B_SKEW		64
#define BDIFF_SKEW	128

#define BUFSIZE ((MAX_STRLEN + 1) * MAX_ALIGN + BDIFF_SKEW)

static char a[BUFSIZE] __attribute__((__aligned__(4096)));
static char b[BUFSIZE] __attribute__((__aligned__(4096)));
static char bdiff[BUFSIZE] __attribute__((__aligned__(4096)));

/* The 32-bit variants are not available on every Arm target, so use the same
   availability check as string/test/strcmp.c.  */
#if __arm__ && __ARM_ARCH >= 7 && __ARM_ARCH_ISA_ARM >= 1
# define RUNSTRCMP32(BENCH) RUN (BENCH, __strcmp_arm)
#elif __arm__ && __ARM_ARCH == 6 && __ARM_ARCH_6M__ >= 1
# define RUNSTRCMP32(BENCH) RUN (BENCH, __strcmp_armv6m)
#else
# define RUNSTRCMP32(BENCH)
#endif

#define DOTEST(STR,TESTFN)			\
  printf (STR);					\
  RUN (TESTFN, strcmp);				\
  RUNA64 (TESTFN, __strcmp_aarch64);		\
  RUNSVE (TESTFN, __strcmp_aarch64_sve);	\
  RUNSTRCMP32 (TESTFN);				\
  printf ("\n");

static uint16_t strcmp_tests[NUM_TESTS];

typedef struct { uint16_t size; uint16_t freq; } freq_data_t;
typedef struct { uint8_t align; uint16_t freq; } align_data_t;

#define SIZE_NUM 65536
#define SIZE_MASK (SIZE_NUM - 1)
static uint8_t strcmp_len_arr[SIZE_NUM];

/* Frequency data for string lengths up to 128 based on SPEC2017.  This is the
   same distribution used by the strlen benchmark.  */
static freq_data_t strcmp_len_freq[] =
{
  { 12,22671}, { 18,12834}, { 13, 9555}, {  6, 6348}, { 17, 6095}, { 11, 2115},
  { 10, 1335}, {  7,  814}, {  2,  646}, {  9,  483}, {  8,  471}, { 16,  418},
  {  4,  390}, {  1,  388}, {  5,  233}, {  3,  204}, {  0,   79}, { 14,   79},
  { 15,   69}, { 26,   36}, { 22,   35}, { 31,   24}, { 32,   24}, { 19,   21},
  { 25,   17}, { 28,   15}, { 21,   14}, { 33,   14}, { 20,   13}, { 24,    9},
  { 29,    9}, { 30,    9}, { 23,    7}, { 34,    7}, { 27,    6}, { 44,    5},
  { 42,    4}, { 45,    3}, { 47,    3}, { 40,    2}, { 41,    2}, { 43,    2},
  { 58,    2}, { 78,    2}, { 36,    2}, { 48,    1}, { 52,    1}, { 60,    1},
  { 64,    1}, { 56,    1}, { 76,    1}, { 68,    1}, { 80,    1}, { 84,    1},
  { 72,    1}, { 86,    1}, { 35,    1}, { 39,    1}, { 50,    1}, { 38,    1},
  { 37,    1}, { 46,    1}, { 98,    1}, {102,    1}, {128,    1}, { 51,    1},
  {107,    1}, { 0,     0}
};

#define ALIGN_NUM 1024
#define ALIGN_MASK (ALIGN_NUM - 1)
static uint8_t strcmp_align_arr[ALIGN_NUM];

/* Alignment data based on SPEC2017.  */
static align_data_t string_align_freq[] =
{
  {8, 470}, {32, 427}, {16, 99}, {1, 19}, {2, 6}, {4, 3}, {0, 0}
};

static uint64_t strcmp_size;

static void
init_strcmp_distribution (void)
{
  int i, j, freq, size, n;

  for (n = i = 0; (freq = strcmp_len_freq[i].freq) != 0; i++)
    for (j = 0, size = strcmp_len_freq[i].size; j < freq; j++)
      strcmp_len_arr[n++] = size;
  assert (n == SIZE_NUM);

  for (n = i = 0; (freq = string_align_freq[i].freq) != 0; i++)
    for (j = 0, size = string_align_freq[i].align; j < freq; j++)
      strcmp_align_arr[n++] = size;
  assert (n == ALIGN_NUM);
}

static void
init_strcmp_tests (void)
{
  uint16_t index[MAX_ALIGN];

  memset (a, 'x', sizeof (a));

  /* Create indices for strings at all alignments.  */
  for (int i = 0; i < MAX_ALIGN; i++)
    {
      index[i] = i * (MAX_STRLEN + 1);
      a[index[i] + MAX_STRLEN] = 0;
    }

  /* b is identical to a, so a comparison scans the whole string and stops on
     the terminating null.  bdiff differs from a in the last character before
     the null of every string, so a comparison scans the whole string and stops
     on a mismatch instead.  All strings sharing an alignment slot end on the
     same byte, so one flipped byte per slot covers every string in it.  */
  memcpy (b + B_SKEW, a, sizeof (a) - BDIFF_SKEW);
  memcpy (bdiff + BDIFF_SKEW, a, sizeof (a) - BDIFF_SKEW);
  for (int i = 0; i < MAX_ALIGN; i++)
    bdiff[BDIFF_SKEW + index[i] + MAX_STRLEN - 1] = 'y';

  /* Create a random set of input strings using the string length and
     alignment distributions.  */
  for (int n = 0; n < NUM_TESTS; n++)
    {
      int align = strcmp_align_arr[rand32 (0) & ALIGN_MASK];
      int exp_len = strcmp_len_arr[rand32 (0) & SIZE_MASK];

      strcmp_tests[n] =
	index[(align + exp_len) & (MAX_ALIGN - 1)] + MAX_STRLEN - exp_len;
      assert ((strcmp_tests[n] & (align - 1)) == 0);
      assert (strlen (a + strcmp_tests[n]) == (size_t) exp_len);
      assert (strcmp (a + strcmp_tests[n],
		      b + B_SKEW + strcmp_tests[n]) == 0);
      assert (exp_len == 0
	      || strcmp (a + strcmp_tests[n],
			 bdiff + BDIFF_SKEW + strcmp_tests[n]) != 0);

      /* Bytes inspected by one pass: the string plus its terminator.  */
      strcmp_size += exp_len + 1;
    }
}

static volatile size_t maskv = 0;

/* Equal strings, so every call scans to the terminating null.  */
static void inline __attribute ((always_inline))
strcmp_random_equal (const char *name, int (*fn)(const char *, const char *))
{
  size_t res = 0, mask = maskv;
  uint64_t total = strcmp_size * ITERS_RANDOM;
  printf ("%22s ", name);

  /* Measure throughput of strcmp.  */
  uint64_t t = clock_get_ns ();
  for (int i = 0; i < ITERS_RANDOM; i++)
    for (int c = 0; c < NUM_TESTS; c++)
      res += fn (a + strcmp_tests[c], b + B_SKEW + strcmp_tests[c]);
  t = clock_get_ns () - t;
  printf ("tp: %.3f ", (double) total / t);

  /* Measure latency by feeding the result back into the address.  */
  t = clock_get_ns ();
  for (int i = 0; i < ITERS_RANDOM; i++)
    for (int c = 0; c < NUM_TESTS; c++)
      res += fn (a + strcmp_tests[c] + (res & mask),
		 b + B_SKEW + strcmp_tests[c] + (res & mask));
  t = clock_get_ns () - t;
  printf ("lat: %.3f\n", (double) total / t);
  maskv = res & mask;
}

/* Strings differing in the last character, so every call scans the whole
   string and exits on a mismatch rather than on the null.  */
static void inline __attribute ((always_inline))
strcmp_random_diff (const char *name, int (*fn)(const char *, const char *))
{
  size_t res = 0, mask = maskv;
  uint64_t total = strcmp_size * ITERS_RANDOM;
  printf ("%22s ", name);

  uint64_t t = clock_get_ns ();
  for (int i = 0; i < ITERS_RANDOM; i++)
    for (int c = 0; c < NUM_TESTS; c++)
      res += fn (a + strcmp_tests[c], bdiff + BDIFF_SKEW + strcmp_tests[c]);
  t = clock_get_ns () - t;
  printf ("tp: %.3f ", (double) total / t);

  t = clock_get_ns ();
  for (int i = 0; i < ITERS_RANDOM; i++)
    for (int c = 0; c < NUM_TESTS; c++)
      res += fn (a + strcmp_tests[c] + (res & mask),
		 bdiff + BDIFF_SKEW + strcmp_tests[c] + (res & mask));
  t = clock_get_ns () - t;
  printf ("lat: %.3f\n", (double) total / t);
  maskv = res & mask;
}

static void inline __attribute ((always_inline))
strcmp_fixed (const char *name, int (*fn)(const char *, const char *),
	      int minsize, int maxsize, int iters, int off)
{
  size_t res = 0, mask = maskv;
  printf ("%22s ", name);

  for (int size = minsize; size <= maxsize; size *= 2)
    {
      memset (a, 'x', size);
      a[size - 1] = 0;
      memset (b + B_SKEW + off, 'x', size);
      b[B_SKEW + off + size - 1] = 0;

      uint64_t t = clock_get_ns ();
      for (int i = 0; i < iters; i++)
	res += fn (a + (i & mask), b + B_SKEW + off + (i & mask));
      t = clock_get_ns () - t;
      printf ("%d%c: %5.2f ", size < 1024 ? size : size / 1024,
	      size < 1024 ? 'B' : 'K', (double) size * iters / t);
    }
  maskv &= res;
  printf ("\n");
}

static void inline __attribute ((always_inline))
strcmp_small_aligned (const char *name, int (*fn)(const char *, const char *))
{
  strcmp_fixed (name, fn, 1, 64, ITERS_SMALL, 0);
}

static void inline __attribute ((always_inline))
strcmp_small_misaligned (const char *name, int (*fn)(const char *,
						     const char *))
{
  strcmp_fixed (name, fn, 1, 64, ITERS_SMALL, MISALIGN);
}

static void inline __attribute ((always_inline))
strcmp_medium_aligned (const char *name, int (*fn)(const char *, const char *))
{
  strcmp_fixed (name, fn, 128, 4096, ITERS_MEDIUM, 0);
}

static void inline __attribute ((always_inline))
strcmp_medium_misaligned (const char *name, int (*fn)(const char *,
						      const char *))
{
  strcmp_fixed (name, fn, 128, 4096, ITERS_MEDIUM, MISALIGN);
}

int main (void)
{
  rand32 (0x12345678);
  init_strcmp_distribution ();
  init_strcmp_tests ();

  DOTEST ("Random strcmp, equal (bytes/ns):\n", strcmp_random_equal);
  DOTEST ("Random strcmp, mismatch (bytes/ns):\n", strcmp_random_diff);
  DOTEST ("Small aligned strcmp (bytes/ns):\n", strcmp_small_aligned);
  DOTEST ("Small misaligned strcmp (bytes/ns):\n", strcmp_small_misaligned);
  DOTEST ("Medium aligned strcmp (bytes/ns):\n", strcmp_medium_aligned);
  DOTEST ("Medium misaligned strcmp (bytes/ns):\n", strcmp_medium_misaligned);

  return 0;
}
