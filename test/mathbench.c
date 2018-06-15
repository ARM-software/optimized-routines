/*
 * Microbenchmark for math functions.
 *
 * Copyright (c) 2018, Arm Limited.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#undef _GNU_SOURCE
#define _GNU_SOURCE 1
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "mathlib.h"

/* Number of measurements, best result is reported.  */
#define MEASURE 60
/* Array size.  */
#define N 8000
/* Iterations over the array.  */
#define ITER 125

static double *Trace;
static size_t trace_size;
static double A[N];
static float Af[N];
static long measurecount = MEASURE;
static long itercount = ITER;

static double
dummy (double x)
{
  return x;
}

static float
dummyf (float x)
{
  return x;
}

static double
xypow (double x)
{
  return pow (x, x);
}

static float
xypowf (float x)
{
  return powf (x, x);
}

static double
xpow (double x)
{
  return pow (x, 23.4);
}

static float
xpowf (float x)
{
  return powf (x, 23.4f);
}

static double
ypow (double x)
{
  return pow (2.34, x);
}

static float
ypowf (float x)
{
  return powf (2.34f, x);
}

static float
sincosf_wrap (float x)
{
  float s, c;
  sincosf (x, &s, &c);
  return s + c;
}

static const struct fun
{
  const char *name;
  int prec;
  double lo;
  double hi;
  union
  {
    double (*d) (double);
    float (*f) (float);
  } fun;
} funtab[] = {
#define D(func, lo, hi) {#func, 'd', lo, hi, {.d = func}},
#define F(func, lo, hi) {#func, 'f', lo, hi, {.f = func}},
D (dummy, 1.0, 2.0)
D (exp, -9.9, 9.9)
D (exp, 0.5, 1.0)
D (exp2, -9.9, 9.9)
D (log, 0.01, 11.1)
D (log, 0.999, 1.001)
D (log2, 0.01, 11.1)
D (log2, 0.999, 1.001)
{"pow", 'd', 0.01, 11.1, {.d = xypow}},
D (xpow, 0.01, 11.1)
D (ypow, -9.9, 9.9)

F (dummyf, 1.0, 2.0)
F (expf, -9.9, 9.9)
F (exp2f, -9.9, 9.9)
F (logf, 0.01, 11.1)
F (log2f, 0.01, 11.1)
{"powf", 'f', 0.01, 11.1, {.f = xypowf}},
F (xpowf, 0.01, 11.1)
F (ypowf, -9.9, 9.9)
{"sincosf", 'f', 0.1, 0.7, {.f = sincosf_wrap}},
{"sincosf", 'f', 0.8, 3.1, {.f = sincosf_wrap}},
{"sincosf", 'f', -3.1, 3.1, {.f = sincosf_wrap}},
{"sincosf", 'f', 3.3, 33.3, {.f = sincosf_wrap}},
{"sincosf", 'f', 100, 1000, {.f = sincosf_wrap}},
{"sincosf", 'f', 1e6, 1e32, {.f = sincosf_wrap}},
F (sinf, 0.1, 0.7)
F (sinf, 0.8, 3.1)
F (sinf, -3.1, 3.1)
F (sinf, 3.3, 33.3)
F (sinf, 100, 1000)
F (sinf, 1e6, 1e32)
F (cosf, 0.1, 0.7)
F (cosf, 0.8, 3.1)
F (cosf, -3.1, 3.1)
F (cosf, 3.3, 33.3)
F (cosf, 100, 1000)
F (cosf, 1e6, 1e32)
{0},
#undef F
#undef D
};

static void
gen_linear (double lo, double hi)
{
  for (int i = 0; i < N; i++)
    A[i] = (lo * (N - i) + hi * i) / N;
}

static void
genf_linear (double lo, double hi)
{
  for (int i = 0; i < N; i++)
    Af[i] = (float)(lo * (N - i) + hi * i) / N;
}

static inline double
asdouble (uint64_t i)
{
  union
  {
    uint64_t i;
    double f;
  } u = {i};
  return u.f;
}

static uint64_t seed = 0x0123456789abcdef;

static double
frand (double lo, double hi)
{
  seed = 6364136223846793005ULL * seed + 1;
  return lo + (hi - lo) * (asdouble (seed >> 12 | 0x3ffULL << 52) - 1.0);
}

static void
gen_rand (double lo, double hi)
{
  for (int i = 0; i < N; i++)
    A[i] = frand (lo, hi);
}

static void
genf_rand (double lo, double hi)
{
  for (int i = 0; i < N; i++)
    Af[i] = (float)frand (lo, hi);
}

static void
gen_trace (int index)
{
  for (int i = 0; i < N; i++)
    A[i] = Trace[index + i];
}

static void
genf_trace (int index)
{
  for (int i = 0; i < N; i++)
    Af[i] = (float)Trace[index + i];
}

static void
run_thruput (double f (double))
{
  for (int i = 0; i < N; i++)
    f (A[i]);
}

static void
runf_thruput (float f (float))
{
  for (int i = 0; i < N; i++)
    f (Af[i]);
}

volatile double zero = 0;

static void
run_latency (double f (double))
{
  double z = zero;
  double prev = z;
  for (int i = 0; i < N; i++)
    prev = f (A[i] + prev * z);
}

static void
runf_latency (float f (float))
{
  float z = (float)zero;
  float prev = z;
  for (int i = 0; i < N; i++)
    prev = f (Af[i] + prev * z);
}

static uint64_t
tic (void)
{
  struct timespec ts;
  if (clock_gettime (CLOCK_REALTIME, &ts))
    abort ();
  return ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

#define TIMEIT(run, f) do { \
  dt = -1; \
  run (f); /* Warm up.  */ \
  for (int j = 0; j < measurecount; j++) \
    { \
      uint64_t t0 = tic (); \
      for (int i = 0; i < itercount; i++) \
	run (f); \
      uint64_t t1 = tic (); \
      if (t1 - t0 < dt) \
	dt = t1 - t0; \
    } \
} while (0)

static void
bench1 (const struct fun *f, int type, double lo, double hi)
{
  uint64_t dt = 0;
  uint64_t ns100;
  const char *s = type == 't' ? "rthruput" : "latency";

  if (f->prec == 'd' && type == 't')
    TIMEIT (run_thruput, f->fun.d);
  else if (f->prec == 'd' && type == 'l')
    TIMEIT (run_latency, f->fun.d);
  else if (f->prec == 'f' && type == 't')
    TIMEIT (runf_thruput, f->fun.f);
  else if (f->prec == 'f' && type == 'l')
    TIMEIT (runf_latency, f->fun.f);

  ns100 = (100 * dt + itercount * N / 2) / (itercount * N);
  printf ("%7s %8s: %4u.%02u ns/elem %10llu ns in [%g %g]\n", f->name, s,
	  (unsigned) (ns100 / 100), (unsigned) (ns100 % 100),
	  (unsigned long long) dt, lo, hi);
}

static void
bench (const struct fun *f, double lo, double hi, int type, int gen)
{
  if (f->prec == 'd' && gen == 'r')
    gen_rand (lo, hi);
  else if (f->prec == 'd' && gen == 'l')
    gen_linear (lo, hi);
  else if (f->prec == 'd' && gen == 't')
    gen_trace (0);
  else if (f->prec == 'f' && gen == 'r')
    genf_rand (lo, hi);
  else if (f->prec == 'f' && gen == 'l')
    genf_linear (lo, hi);
  else if (f->prec == 'f' && gen == 't')
    genf_trace (0);

  if (gen == 't')
    hi = trace_size / N;

  if (type == 'b' || type == 't')
    bench1 (f, 't', lo, hi);

  if (type == 'b' || type == 'l')
    bench1 (f, 'l', lo, hi);

  for (int i = N; i < trace_size; i += N)
    {
      if (f->prec == 'd')
	gen_trace (i);
      else
	genf_trace (i);

      lo = i / N;
      if (type == 'b' || type == 't')
	bench1 (f, 't', lo, hi);

      if (type == 'b' || type == 'l')
	bench1 (f, 'l', lo, hi);
    }
}

static void
readtrace (const char *name)
{
	int n = 0;
	FILE *f = strcmp (name, "-") == 0 ? stdin : fopen (name, "r");
	if (!f)
	  {
	    printf ("openning \"%s\" failed: %m\n", name);
	    exit (1);
	  }
	for (;;)
	  {
	    if (n >= trace_size)
	      {
		trace_size += N;
		Trace = realloc (Trace, trace_size * sizeof (Trace[0]));
		if (Trace == NULL)
		  {
		    printf ("out of memory\n");
		    exit (1);
		  }
	      }
	    if (fscanf (f, "%lf", Trace + n) != 1)
	      break;
	    n++;
	  }
	if (ferror (f) || n == 0)
	  {
	    printf ("reading \"%s\" failed: %m\n", name);
	    exit (1);
	  }
	fclose (f);
	if (n % N == 0)
	  trace_size = n;
	for (int i = 0; n < trace_size; n++, i++)
	  Trace[n] = Trace[i];
}

static void
usage (void)
{
  printf ("usage: ./mathbench [-g rand|linear|trace] [-t latency|thruput|both] "
	  "[-i low high] [-f tracefile] [-m measurements] [-c iterations] func "
	  "[func2 ..]\n");
  printf ("func:\n");
  printf ("%7s [run all benchmarks]\n", "all");
  for (const struct fun *f = funtab; f->name; f++)
    printf ("%7s [low: %g high: %g]\n", f->name, f->lo, f->hi);
  exit (1);
}

int
main (int argc, char *argv[])
{
  int usergen = 0, gen = 'r', type = 'b', all = 0;
  double lo = 0, hi = 0;
  const char *tracefile = "-";

  argv++;
  argc--;
  for (;;)
    {
      if (argc <= 0)
	usage ();
      if (argv[0][0] != '-')
	break;
      else if (argc >= 3 && strcmp (argv[0], "-i") == 0)
	{
	  usergen = 1;
	  lo = strtod (argv[1], 0);
	  hi = strtod (argv[2], 0);
	  argv += 3;
	  argc -= 3;
	}
      else if (argc >= 2 && strcmp (argv[0], "-m") == 0)
	{
	  measurecount = strtol (argv[1], 0, 0);
	  argv += 2;
	  argc -= 2;
	}
      else if (argc >= 2 && strcmp (argv[0], "-c") == 0)
	{
	  itercount = strtol (argv[1], 0, 0);
	  argv += 2;
	  argc -= 2;
	}
      else if (argc >= 2 && strcmp (argv[0], "-g") == 0)
	{
	  gen = argv[1][0];
	  if (strchr ("rlt", gen) == 0)
	    usage ();
	  argv += 2;
	  argc -= 2;
	}
      else if (argc >= 2 && strcmp (argv[0], "-f") == 0)
	{
	  gen = 't';  /* -f implies -g trace.  */
	  tracefile = argv[1];
	  argv += 2;
	  argc -= 2;
	}
      else if (argc >= 2 && strcmp (argv[0], "-t") == 0)
	{
	  type = argv[1][0];
	  if (strchr ("ltb", type) == 0)
	    usage ();
	  argv += 2;
	  argc -= 2;
	}
      else
	usage ();
    }
  if (gen == 't')
    {
      readtrace (tracefile);
      lo = hi = 0;
      usergen = 1;
    }
  while (argc > 0)
    {
      int found = 0;
      all = strcmp (argv[0], "all") == 0;
      for (const struct fun *f = funtab; f->name; f++)
	if (all || strcmp (argv[0], f->name) == 0)
	  {
	    found = 1;
	    if (!usergen)
	      {
		lo = f->lo;
		hi = f->hi;
	      }
	    bench (f, lo, hi, type, gen);
	    if (usergen && !all)
	      break;
	  }
      if (!found)
	printf ("unknown function: %s\n", argv[0]);
      argv++;
      argc--;
    }
  return 0;
}
