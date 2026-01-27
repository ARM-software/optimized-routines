/*
 * Vectorised fallback for Double-Precision AdvSIMD trig functions.
 *
 * Copyright (c) 2026, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

static const struct fallback_data
{
  float64x2_t shift;
  float64x2_t tm48, offset;
  double hp0, hp1;
  double mp0, mp1;
  double sincos_poly[14];
  double split;
} fallback_data = {
  .split = +0x1.0000002p+27,
  .hp0 = 0x1.921fb54442d18p+0,
  .hp1 = 0x1.1a62633145c07p-54,
  .mp0 = 0x1.921fb58000000p+0,
  .mp1 = -0x1.dde9740000000p-27,
  .offset = { 1.0, 0x1p-24 },
  .tm48 = V2 (0x1p-48),
  .shift = V2 (0x1.8p54),

  .sincos_poly = {
    /* Sine polynomial.       Cosine Polynomial.  */
    -0x1.555555555547bp-3,    -0x1p-1,
     0x1.1111111108a4dp-7,    0x1.555555555554cp-5,
    -0x1.a01a019936f27p-13,   -0x1.6c16c16c1521fp-10,
     0x1.71de37a97d93ep-19,   0x1.a01a019cbf62ap-16,
    -0x1.ae633919987c6p-26,   -0x1.27e4f812b681ep-22,
     0x1.60e277ae07cecp-33,   0x1.1ee9f152a57cdp-29,
    -0x1.9e9540300a100p-41,   -0x1.8fb131098404bp-37,
  },
};

/* A table storing a high precision 2 / pi, computed in arbitrary precision.
   This table is stored in a struct separate from the rest of the fallback
   data, as LDP can only take offsets as high as 512 bytes, and so more LDR's
   are used.  */
static const struct lookup_table
{
  double inv_pio2[75];
} table = {
  .inv_pio2 = {
    0x1.45f306p23, 0x1.b93910p22, 0x1.529fc0p20, 0x1.3abe88p21, 0x1.ea69bap23,
    0x1.81b6c4p23, 0x1.2b3278p23, 0x1.0e4104p22, 0x1.fca2c6p23, 0x1.57bd76p23,
    0x1.8ac36ep23, 0x1.2371d0p21, 0x1.093748p22, 0x1.c00c92p23, 0x1.775048p21,
    0x1.a32438p23, 0x1.fc3bd6p23, 0x1.cb1290p20, 0x1.4e7dd0p23, 0x1.046beap23,
    0x1.75da20p21, 0x1.09d338p23, 0x1.c09ad0p22, 0x1.7df904p22, 0x1.cc8eb0p21,
    0x1.cc1a98p21, 0x1.cfa4e0p21, 0x1.08bf16p23, 0x1.7bf250p23, 0x1.d8ffc0p21,
    0x1.2fffbcp23, 0x1.6603c0p18, 0x1.de5e22p23, 0x1.16b414p23, 0x1.b47db4p22,
    0x1.b3f678p21, 0x1.3e5848p21, 0x1.6e9e8cp23, 0x1.fb34f0p21, 0x1.7fa8b4p22,
    0x1.d49ee8p22, 0x1.8fd7cap23, 0x1.e2f67ap23, 0x1.ce7dc0p18, 0x1.14a524p23,
    0x1.d4d7f6p23, 0x1.7ec47cp22, 0x1.1aba10p23, 0x1.580cc0p22, 0x1.1bf1ecp22,
    0x1.aeafc0p22, 0x1.9f7840p23, 0x1.35e86cp23, 0x1.da9e30p20, 0x1.22c2bcp23,
    0x1.cc3610p23, 0x1.966614p22, 0x1.7c5280p22, 0x1.a10234p22, 0x1.ffb100p23,
    0x1.35cc9cp22, 0x1.883030p21, 0x1.556ca0p20, 0x1.cea324p22, 0x1.8389ecp22,
    0x1.8118d6p23, 0x1.1f1064p22, 0x1.86cf9ap23, 0x1.b9d012p23, 0x1.541ac8p21,
    0x1.88ed16p23, 0x1.2c394cp23, 0x1.bb5e88p23, 0x1.a2ae32p23, 0x1.4fa940p18,
  },
};

/* Uses a shift value of 0x1.8p54 to round to multiples of four without
   use of rounding instructions/intrinsics.  */
static inline float64x2_t VPCS_ATTR
round_to_multiples_of_four (float64x2_t x)
{
  const struct fallback_data *d = ptr_barrier (&fallback_data);
  return vsubq_f64 (vaddq_f64 (x, d->shift), d->shift);
}

/* A struct representing a double-double value.  */
struct dd_t
{
  double hi, lo;
};

/* The reduction algorithm computes a remainder (double-double)
    and a quotient (integer) such that
    x * pi/2 = quotient + (remainder.hi + remainder.lo)
    The quotient is reduced to a circle quadrant.  */
struct reduced_xpio2_t
{
  struct dd_t remainder;
  int quadrant; /* quotient & 3.  */
};

/* Uses Dekker's algorithm to split x into two variables containing
    the higher lower order (hi/lo) bits of the input.  */
static inline struct dd_t VPCS_ATTR
to_dd_accurate (double x)
{
  const struct fallback_data *d = ptr_barrier (&fallback_data);

  double t = x * d->split;
  double hi = t - (t - x);
  double lo = x - hi;

  opt_barrier_double (t); /* Prevent compiler optimisation.  */

  return (struct dd_t){ hi, lo };
}

/* A faster version of the split algorithm, but the result incurs more
   error. This can be acceptable in some situations.  */
static inline struct dd_t VPCS_ATTR
to_dd_fast (double x)
{
  const struct fallback_data *d = ptr_barrier (&fallback_data);

  double t = (x * d->split) - x;
  double hi = (x * d->split) - t;
  double lo = x - hi;

  return (struct dd_t){ hi, lo };
}

struct inv_pio2_block
{
  float64x2_t hi, mi, lo;
};

/* Takes a single index k, and returns [k..k+6] from the 2 /pi table.  */
static inline struct inv_pio2_block VPCS_ATTR
lookup_block (int k)
{
  const struct lookup_table *d = ptr_barrier (&table);
  struct inv_pio2_block ret;
  ret.hi = vld1q_f64 (&d->inv_pio2[k + 0]);
  ret.mi = vld1q_f64 (&d->inv_pio2[k + 2]);
  ret.lo = vld1q_f64 (&d->inv_pio2[k + 4]);

  return ret;
}

/* Unlike the rest of this algorithm, where the two lanes of a vector are used
   to compute upper and lower magnitudes of x, this function computes each
   magnitude separately. Vectorisation here is instead used to accelerate the
   computation of each magnitude, as it allows for more efficient loading of
   data from the 2 / pi lookup table.

   This function computes x * (2 / pi), in six individual 24 bit blocks.
   The result can be thought of as a 6 digit base 2^24 number, i.e:
    d0 + d1*2^24 + d2*2^48 + ...

   24 bits are used per partial piece, as the maximum result of multiplying two
   24 bit numbers would requires 48 bits to store, which perfectly fits within
   double precision's 53 bit mantissa.  */
static inline void VPCS_ATTR
compute_x_times_invpio2 (float64x2_t *data, double d_x)
{
  const struct fallback_data *d = ptr_barrier (&fallback_data);
  int k1, k2;

  /* Scales down the input value (which is assumed to be large)
     to avoid overflow of intermediate values.  */
  double d_x_scaled = d_x * 0x1p-600;

  /* Splits x into a higher/lower magnitude pair,
     and then pack into a vector for double-double reduction.  */
  struct dd_t x_split = to_dd_accurate (d_x_scaled);
  float64x2_t x_scaled = { x_split.hi, x_split.lo };

  /* Calculates the table index for each lane to use.  */
  k1 = (asuint64 (x_split.hi) >> 52) & 0x7ff;
  k2 = (asuint64 (x_split.lo) >> 52) & 0x7ff;
  k1 = (k1 - 450) / 24;
  k2 = (k2 - 450) / 24;

  k1 = k1 < 0 ? 0 : k1;
  k2 = k2 < 0 ? 0 : k2;

  /* Elements in the table need to be rescaled depending on their position,
     so we create a scale factor of 2^(576-24k).  */
  float64x2_t scale_hi
      = v_f64 (asdouble (asuint64 (0x1p576) - ((uint64_t) (k1 * 24) << 52)));
  float64x2_t scale_lo
      = v_f64 (asdouble (asuint64 (0x1p576) - ((uint64_t) (k2 * 24) << 52)));

  /* Since we're using vectors to accelerate this, we multiply the scale by
     <1, 0x1p-24> This causes the second lane to act as if it's one iteration
     ahead, halving the iterations required per vector.  */
  scale_hi = vmulq_f64 (scale_hi, d->offset);
  scale_lo = vmulq_f64 (scale_lo, d->offset);

  float64x2_t result[6];
  float64x2_t x_hi, x_lo;

  x_hi = vmulq_laneq_f64 (scale_hi, x_scaled, 0);
  x_lo = vmulq_laneq_f64 (scale_lo, x_scaled, 1);

  struct inv_pio2_block inv_pio2_hi = lookup_block (k1);
  struct inv_pio2_block inv_pio2_lo = lookup_block (k2);

  result[0] = vmulq_f64 (x_hi, inv_pio2_hi.hi);
  x_hi = vmulq_f64 (x_hi, d->tm48);
  result[1] = vmulq_f64 (x_hi, inv_pio2_hi.mi);
  x_hi = vmulq_f64 (x_hi, d->tm48);
  result[2] = vmulq_f64 (x_hi, inv_pio2_hi.lo);

  result[3] = vmulq_f64 (x_lo, inv_pio2_lo.hi);
  x_lo = vmulq_f64 (x_lo, d->tm48);
  result[4] = vmulq_f64 (x_lo, inv_pio2_lo.mi);
  x_lo = vmulq_f64 (x_lo, d->tm48);
  result[5] = vmulq_f64 (x_lo, inv_pio2_lo.lo);

  /* Since this function used vectors to accelerate each magnitude of x, rather
     than to compute both magnitudes alongside each other, we have to transpose
     the vectors before returning.  */
  data[0] = vtrn1q_f64 (result[0], result[3]);
  data[1] = vtrn2q_f64 (result[0], result[3]);
  data[3] = vtrn2q_f64 (result[1], result[4]);
  data[2] = vtrn1q_f64 (result[1], result[4]);
  data[4] = vtrn1q_f64 (result[2], result[5]);
  data[5] = vtrn2q_f64 (result[2], result[5]);
}

/* Reduce x * (2/pi) into:
    x * (2/pi) = int_multiple + (fraction_high + fraction_low),
   where fraction_high/low are computed in each lane of the input vector.

   The reduction is computed in an extended-precision, using double-double
   arithmetic, using 6 blocks of 24 bits.

   The algorithm can be broken up into four steps:
   1. Calls compute_x_times_invpio2, which uses six 24-bit blocks to compute
      partial sums of a 144 bit precision value of x*(2/pi).
   2. Separate integer and fractional accumulation, to prevent catastrophic
      cancellation errors that would occur if summed directly due to magnitude
      differences.
   3. After removing integer components, re-splitting hi/lo values recovers
      precision lost during intermediate rounding, yielding a minimal-error
      reduced remainder.
   4. The integer sum can be used to determine the quadrant, and the fractional
      remainder is returned as a hi/lo pair, to maintain the
      extended precision.  */
static inline struct reduced_xpio2_t VPCS_ATTR
vec_extended_range_reduction (double x)
{
  float64x2_t r[6];
  compute_x_times_invpio2 (&r[0], x);

  /* A sum to hold all the integer portions of the reduction.  */
  float64x2_t sum = { 0, 0 };

  /* Accumulates the integer component of the high reduction values,
     Stores the result in sum, and subtracts the integer from r, leaving
     r with just the fractional remainder of the reduction.  */
  for (int i = 0; i < 3; i++)
    {
      float64x2_t s = vrndaq_f64 (r[i]);
      sum = vaddq_f64 (sum, s);
      r[i] = vsubq_f64 (r[i], s);
    }

  /* Since r now only contains the fractional components of the reduction,
     we can accumulate the values together into a tail.  */
  float64x2_t tail_hi;
  /* First two additions can be parallelised
     without noticeable accuracy loss.  */
  float64x2_t t0 = vaddq_f64 (r[5], r[4]);
  float64x2_t t1 = vaddq_f64 (r[3], r[2]);

  tail_hi = vaddq_f64 (t0, t1);
  tail_hi = vaddq_f64 (r[1], tail_hi);
  tail_hi = vaddq_f64 (r[0], tail_hi);

  /* By subtracting the total tail from the largest magnitude of r,
     and reaccumulating the lower magnitudes, we can construct a hi/lo
     double-double pair for the tail value.  */
  float64x2_t tail_lo;
  tail_lo = vsubq_f64 (r[0], tail_hi);
  tail_lo = vaddq_f64 (tail_lo, r[1]);
  tail_lo = vaddq_f64 (tail_lo, r[2]);
  tail_lo = vaddq_f64 (tail_lo, r[3]);

  /* Final two additions can be parallelised
     without noticeable accuracy loss.  */
  tail_lo = vaddq_f64 (tail_lo, vaddq_f64 (r[4], r[5]));

  /* The high magnitude of tail might end up accumulating high enough to have
     an integer component, which can be removed and added to the sum.  */
  float64x2_t s = vrndaq_f64 (tail_hi);
  sum = vaddq_f64 (sum, s);
  tail_hi = vsubq_f64 (tail_hi, s);

  /* If tail_hi had an integer component removed, it's magnitude will be
     lowered. We can recombine hi/lo together, and then re-split them to get
     better precision.  */
  float64x2_t tail = vaddq_f64 (tail_hi, tail_lo);
  float64x2_t tail_err = vsubq_f64 (tail_hi, tail);
  tail_lo = vaddq_f64 (tail_err, tail_lo);

  s = round_to_multiples_of_four (sum);
  sum = vsubq_f64 (sum, s);

  /* Since the vector contains hi/lo magnitudes of the reduction, we need to
     sum them together to get a scalar value to return.  */
  double s_sum = vpaddd_f64 (sum);
  double s_tail_hi = vpaddd_f64 (tail);
  double s_tail_lo = vpaddd_f64 (tail_lo);

  /* We can do a final addition to tail_lo, to correct for the magnitude
     changes of adding the two vector lanes together.  */
  float64x2_t abs_tail = vabsq_f64 (tail);
  double hi, lo;
  if (abs_tail[0] > abs_tail[1])
    {
      hi = tail[0];
      lo = tail[1];
    }
  else
    {
      hi = tail[1];
      lo = tail[0];
    }

  s_tail_lo += (hi - s_tail_hi) + lo;

  /* Since sum contains the integer component, tail_hi should only contain a
     fractional part. However, if |tail_hi| > 0.5, we can add/subtract 1 to
     tail_hi and sum (depending on sign) in order to keep the tail portion as
     small as possible.  */
  if (s_tail_hi > 0.5)
    {
      s_tail_hi -= 1.0;
      s_sum += 1.0;
    }
  else if (s_tail_hi < -0.5)
    {
      s_tail_hi += 1.0;
      s_sum -= 1.0;
    }

  /* Finally, return the sum + tail_hi + tail_lo reduction of x.  */
  struct reduced_xpio2_t ret;
  struct dd_t rem;
  rem.hi = (s_tail_hi + s_tail_lo);
  rem.lo = (s_tail_hi - rem.hi) + s_tail_lo;

  /* The exact multiple of pi doesn't matter, since we only need to know which
     quadrant the reduction falls in for our approximation,
     so return sum & 3.  */
  ret.quadrant = ((int) s_sum) & 3;
  ret.remainder = rem;
  return ret;
}

/* A fast reduction of trig inputs, using AdvSIMD
   instructions to accelerate the double-double stages.

   Takes a value of x, which is assumed to be large (|x| > 2^23)
   and returns three values - n, r_hi, and r_lo, such that:
    x = n * pi/2 + (r_hi + r_lo),
   where n is an integer, and |r_hi + r_lo| < pi / 4.  */
static inline int VPCS_ATTR
v_large_reduction (double x, struct dd_t *r)
{
  const struct fallback_data *d = ptr_barrier (&fallback_data);

  struct reduced_xpio2_t core = vec_extended_range_reduction (x);

  double s = core.remainder.hi;
  double t = core.remainder.lo;

  /* At this stage of the reduction, core contains a integer quadrant, and a
     double-double fractional remainder s and t, such that:
      s + t = fraction of x * 2 /pi.

     To convert this back to radians, we need to multiply this by pi/2.
     Mathematically, our result should be:
      sum = s + t
      tail = ((s+t) * pi/2) - sum

     However, s will be significantly greater than t after the extended
     reduction, as such, combining s and t directly will lead to rounding
     errors in the tail. To counteract this, we can instead use Dekker
     multiplication. For two doubles, a and b, split them into:
     a = a_hi + a_lo, and b = b_hi + b_lo

     a * b can then be computed via a * b = prod_hi + prod_lo, where p splits
     are:
     prod_hi = a * b_hi prod_lo = ((a_hi * b_hi) - prod_hi)
	      + (a_hi * b_lo) + (a_lo * b_hi) + (a_lo * b_lo).  */

  struct dd_t s_split = to_dd_fast (s);
  double prod_hi = s * d->hp0;

  /* Since s is being multiplied by hp0/1, if we used the same constant for the
     split, Then there will be cancellation in s_hi*hp0 - s * hp0. Therefore,
     the splits of s are multiplied by a slightly different rounding of pi/2.
   */
  double hi_lo = s_split.hi * d->mp1;
  double lo_hi = s_split.lo * d->mp0;

  double hi_terms = s_split.hi * d->mp0 - prod_hi;
  double lo_terms = s_split.lo * d->mp1;

  double prod_lo = hi_terms + hi_lo + lo_hi + lo_terms;

  /* We can add a slight correction to the standard multiplication to help
     preserve more accuracy.  */
  double t_hp0 = t * d->hp0;
  double correction = (s * d->hp1) + t_hp0;
  prod_lo += correction;

  /* Final sum and tail can be computed by combining the hi and lo parts of p:
      s = (s+t) * pi/2 exactly
      t = low magnitude correction.  */
  r->hi = prod_hi + prod_lo;
  r->lo = (prod_hi - r->hi) + prod_lo;
  return core.quadrant;
}

static inline double VPCS_ATTR
do_sincos (int32_t n, struct dd_t r)
{
  const struct fallback_data *d = ptr_barrier (&fallback_data);

  double x = r.hi;
  double dx = r.lo;

  /* Sets the base pointer for the coefficents to use
     from the sincos array.  */
  const double *c = &d->sincos_poly[0];
  c += n & 1;

  double x2 = x * x;
  double x4 = x2 * x2;

  /* Computes sine or cosine, dependant on the quadrant (n).  */
  double t1 = (c[10] * x2) + c[8];
  double t2 = (c[6] * x2) + c[4];
  double t3 = (c[2] * x2) + c[0];

  double poly;
  poly = (c[12] * x4) + t1;
  poly = (poly * x4) + t2;
  poly = (poly * x4) + t3;

  double retval;
  if (n & 1)
    {
      /* cos(x) + correction.  */
      double t = ((poly - 0.5 * (dx)) * (x2) + (dx));
      retval = 1.0 + t;
    }
  else
    {
      /* sin(x) + correction.  */
      double t = ((poly * (x) -0.5 * (dx)) * (x2) + (dx));
      retval = x + t;
    }

  /* Negates the output based on quadrant.  */
  return (n & 2) ? -retval : retval;
}

static inline uint32_t
top12 (float x)
{
  return asuint (x) >> 20;
}

/* Vector fallback for AdvSIMD double precision sin (x).

   This function is designed purely as a fallback for vector sin,
   and only guarantees correct output for inputs where |x| > 2^23.  */
static double VPCS_ATTR NOINLINE UNUSED
v_sin_fallback (double x)
{
  /* sin returns NaN for +-inf and NaN inputs.  */
  if (top12 (x) > 0x7ff8)
    return NAN;

  struct dd_t r;
  /* Reduces the input into three values:
      int32_t   n: The quadrant of the curve the input falls on.
      double r.hi: The rounded reduction value of the input.
      double r.lo: A low magnitude correction term to keep accuracy.  */
  int32_t n = v_large_reduction (x, &r);

  /* Computes the sine or cosine of r, based on the quadrant (n),
     and then applies a small correction term from dr.

     Returns an approximation of sin(x), within 3ULP.  */
  return do_sincos (n, r);
}

/* Vector fallback for AdvSIMD double precision cos (x).

   This function is designed purely as a fallback for vector cos,
   and only guarantees correct output for inputs where |x| > 2^23.  */
static double VPCS_ATTR NOINLINE UNUSED
v_cos_fallback (double x)
{
  /* cos returns NaN for +-inf and NaN inputs.  */
  if (top12 (x) > 0x7ff8)
    return NAN;

  struct dd_t r;
  /* Reduces the input into three values:
      int32_t   n: The quadrant of the curve the input falls on.
      double r.hi: The rounded reduction value of the input.
      double r.lo: A low magnitude correction term to keep accuracy.  */
  int32_t n = v_large_reduction (x, &r);

  /* Computes the sine or cosine of r, based on the quadrant (n),
     and then applies a small correction term from dr.

     Returns an approximation of cos(x), within 3ULP.  */
  return do_sincos (n + 1, r);
}

/* Vector fallback for AdvSIMD double precision sincos (x).

   This function is designed purely as a fallback for vector sincos,
   and only guarantees correct output for inputs where |x| > 2^23.  */
static float64x2_t VPCS_ATTR NOINLINE UNUSED
v_sincos_fallback (double large_x)
{
  const struct fallback_data *d = ptr_barrier (&fallback_data);

  /* cos returns NaN for +-inf and NaN inputs.  */
  if (top12 (large_x) > 0x7ff8)
    return v_f64 (NAN);

  struct dd_t r;
  /* Reduces the input into three values:
      int32_t n: The quadrant of the curve the input falls on.
      double  r: The rounded reduction value of the input.
      double dr: A low magnitude correction term to keep accuracy.  */
  int32_t n = v_large_reduction (large_x, &r);

  float64x2_t c0 = vld1q_f64 (&d->sincos_poly[0]);
  float64x2_t c1 = vld1q_f64 (&d->sincos_poly[2]);
  float64x2_t c2 = vld1q_f64 (&d->sincos_poly[4]);
  float64x2_t c3 = vld1q_f64 (&d->sincos_poly[6]);
  float64x2_t c4 = vld1q_f64 (&d->sincos_poly[8]);
  float64x2_t c5 = vld1q_f64 (&d->sincos_poly[10]);
  float64x2_t c6 = vld1q_f64 (&d->sincos_poly[12]);

  float64x2_t offset = { r.hi, 1.0 };
  float64x2_t x = { r.hi, r.hi };
  float64x2_t dx = { r.lo, r.lo };

  float64x2_t x2 = vmulq_f64 (x, x);
  float64x2_t x4 = vmulq_f64 (x2, x2);

  float64x2_t t3 = vfmaq_f64 (c0, x2, c1);
  float64x2_t t2 = vfmaq_f64 (c2, x2, c3);
  float64x2_t t1 = vfmaq_f64 (c4, x2, c5);

  float64x2_t poly;
  poly = vfmaq_f64 (t1, x4, c6);
  poly = vfmaq_f64 (t2, x4, poly);
  poly = vfmaq_f64 (t3, x4, poly);

  poly = vmulq_f64 (poly, offset);
  poly = vfmaq_f64 (poly, v_f64 (-0.5), dx);
  poly = vfmaq_f64 (dx, x2, poly);

  float64x2_t retval = vaddq_f64 (offset, poly);

  switch (n & 3)
    {
    case 0:
      retval = (float64x2_t){ retval[0], retval[1] };
      break;
    case 1:
      retval = (float64x2_t){ retval[1], -retval[0] };
      break;
    case 2:
      retval = (float64x2_t){ -retval[0], -retval[1] };
      break;
    case 3:
      retval = (float64x2_t){ -retval[1], retval[0] };
      break;
    };

  return retval;
}