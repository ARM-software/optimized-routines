/*
 * Generic functions for ULP error estimation.
 *
 * Copyright (c) 2019-2026, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

/* For each different math function type,
   T(x) should add a different suffix to x.
   RT(x) should add a return type specific suffix to x.  */

#ifdef NEW_RT
#undef NEW_RT

# if USE_MPFR
/* Return log2 of Harrison's ULP at the exact reference, clamped to the
   smallest subnormal spacing.  MPFR uses x = m*2^exp with 1/2 <= |m| < 1.  */
static int RT (ulpscale_mpfr) (mpfr_t x, int ternary)
{
  if (mpfr_regular_p (x))
    {
      mpfr_exp_t e = mpfr_get_exp (x) - RT(prec);
      /* A rounded power of two may represent an exact result just above it.
	 MPFR's ternary result distinguishes that case.  */
      if (mpfr_cmp_si_2exp (x, mpfr_sgn (x), mpfr_get_exp (x) - 1) == 0
	  && (ternary == 0 || ternary * mpfr_sgn (x) > 0))
	--e;
      if (e < RT(emin))
	e = RT(emin) - 1;
      return e;
    }
  if (mpfr_zero_p (x))
    return RT(emin) - 1;
  if (mpfr_inf_p (x))
    return RT(emax) - RT(prec);
  /* NaN.  */
  return 0;
}
# endif

/* As above for the wider floating-point reference yl.  Normally its rounded
   result y has the same binade, so use the cheap exponent bits.  If y is a
   power of two, compare magnitudes to pick its upper or lower spacing.
   Overflowed results and the largest finite result (which directed rounding
   can produce above the overflow boundary) use yl.  If yl itself rounded to
   that power of two, only MPFR's ternary value can tell which side of the
   boundary the exact result was on.  */
static int RT (ulpscale_long) (RT (double) yl, RT (float) y)
{
  int scale;
  if (isinf (y) || RT (fabs) (y) == RT (maxfinite))
    {
      /* At the overflow edge, y cannot reveal the binade of yl.  */
      int e;
      RT (double) m = RT (lfrexp) (yl < 0 ? -yl : yl, &e);
      scale = e - RT (prec) - (m == 0.5);
    }
  else
    {
      scale = RT (ulpscale) (y);
      if (!(RT (asuint) (y) & RT (mantmask))
	  && RT (fabs) (y) >= RT (min_normal)
	  && (yl < 0 ? -yl : yl) <= (RT (double)) RT (fabs) (y))
	--scale;
    }
  return scale < RT (emin) ? RT (emin) - 1 : scale;
}

/* Fill in the error data after the caller's fast acceptance check fails.
   A finite yl may round to infinity in the target type.  */
static inline void RT (set_long_reference) (RT (double) yl,
					    struct RT (ret) * p)
{
  p->ref_finite = isfinite (yl);
  p->ulpexp = p->ref_finite ? RT (ulpscale_long) (yl, p->y) : 0;
  if (!p->ref_finite)
    p->tail = 0;
  else if (isinf (p->y))
    /* Measure the tail from the signed overflow boundary, not infinity.  */
    p->tail = RT (lscalbn) (
	yl - (yl < 0 ? -1 : 1) * ((RT (double)) 2 * RT (halfinf)), -p->ulpexp);
  else
    p->tail = RT (lscalbn) (yl - p->y, -p->ulpexp);
}

/* Form the signed error using Harrison's ULP at the unrounded reference.
   Nearest rounding removes 0.5 ULP from its magnitude; directed modes move
   an error in the rounding direction one representable step toward the
   reference.  A correctly rounded result scores zero.  */
static double RT (ulperr) (RT (float) got, const struct RT (ret) * p, int r,
			   int ignore_zero_sign)
{
  RT(float) want = p->y;
  double d;

  /* Equal results, including matching infinities, have zero error.  */
  if (RT(asuint) (got) == RT(asuint) (want))
    return 0.0;
  /* Muller's ulp (NaN) is NaN.  This test score follows the special-value
     rule in Gladman et al. (Zimmermann): two NaNs match, while a one-sided
     NaN has infinite error.  Non-MPFR also compares signalling status.  */
  if (isnan (got) && isnan (want))
  /* Ignore sign of NaN, and signalling-ness for MPFR.  */
# if USE_MPFR
    return 0;
# else
    return RT (issignaling) (got) == RT (issignaling) (want) ? 0 : INFINITY;
# endif
  if (signbit (got) != signbit (want))
    {
      /* Fall through to ULP calculation if ignoring sign of zero and at
	 exactly one of want and got is non-zero.  */
      if (ignore_zero_sign && want == got)
	return 0.0;
      if (!ignore_zero_sign || (want != 0 && got != 0))
	return INFINITY;
    }
  if (isnan (got) || isnan (want))
    return INFINITY;
  if ((r == FE_DOWNWARD && got < want) || (r == FE_UPWARD && got > want)
      || (r == FE_TOWARDZERO && fabs (got) < fabs (want)))
    got = RT (nextafter) (got, want);
  /* A directed step to an infinite reference is still allowed.  */
  if (!p->ref_finite)
    return RT (asuint) (got) == RT (asuint) (want) ? 0 : INFINITY;
  /* Treat infinity from target overflow as the signed 2^emax boundary.  */
  if (RT (asuint) (got) == RT (asuint) (want))
    d = 0;
  else if (isinf (got))
    d = (signbit (got) ? -1.0 : 1.0) * scalbn (1.0, RT (emax) - p->ulpexp)
	- scalbn ((double) want, -p->ulpexp);
  else if (isinf (want))
    d = scalbn ((double) got, -p->ulpexp)
	- (signbit (want) ? -1.0 : 1.0) * scalbn (1.0, RT (emax) - p->ulpexp);
  else
    d = scalbn ((double) got - (double) want, -p->ulpexp);
  double error = d - p->tail;
  if (r == FE_TONEAREST)
    {
      double excess = fabs (error) - 0.5;
      return excess > 0 ? copysign (excess, error) : 0;
    }
  return error;
}

static int RT(isok) (RT(float) ygot, int exgot, RT(float) ywant, int exwant,
		      int exmay)
{
  return RT(asuint) (ygot) == RT(asuint) (ywant)
	 && ((exgot ^ exwant) & ~exmay) == 0;
}

static int RT(isok_nofenv) (RT(float) ygot, RT(float) ywant)
{
  return RT(asuint) (ygot) == RT(asuint) (ywant);
}
#endif

static inline void T (call_fenv) (const struct fun *f, struct T (args) a,
				  int r, RT (float) * y, int *ex,
				  const struct conf *conf)
{
  if (r != FE_TONEAREST)
    fesetround (r);
  feclearexcept (FE_ALL_EXCEPT);
  *y = T (call) (f, a, conf);
  *ex = fetestexcept (FE_ALL_EXCEPT);
  if (r != FE_TONEAREST)
    fesetround (FE_TONEAREST);
}

static inline void T (call_nofenv) (const struct fun *f, struct T (args) a,
				    int r, RT (float) * y, int *ex,
				    const struct conf *conf)
{
  if (r != FE_TONEAREST)
    fesetround (r);
  *y = T (call) (f, a, conf);
  *ex = 0;
  if (r != FE_TONEAREST)
    fesetround (FE_TONEAREST);
}

static inline int T (call_long_fenv) (const struct fun *f, struct T (args) a,
				      int r, struct RT (ret) * p,
				      RT (float) ygot, int exgot)
{
  if (r != FE_TONEAREST)
    fesetround (r);
  feclearexcept (FE_ALL_EXCEPT);
  volatile struct T(args) va = a; // TODO: barrier
  a = va;
  RT(double) yl = T(call_long) (f, a);
  p->y = (RT(float)) yl;
  volatile RT(float) vy = p->y; // TODO: barrier
  (void) vy;
  p->ex = fetestexcept (FE_ALL_EXCEPT);
  if (r != FE_TONEAREST)
    fesetround (FE_TONEAREST);
  p->ex_may = FE_INEXACT;
  if (RT(isok) (ygot, exgot, p->y, p->ex, p->ex_may))
    return 1;
  RT (set_long_reference) (yl, p);
  if (RT(fabs) (p->y) < RT(min_normal))
    {
      /* TODO: subnormal result is treated as undeflow even if it's
	 exact since call_long may not raise inexact correctly.  */
      if (p->y != 0 || (p->ex & FE_INEXACT))
	p->ex |= FE_UNDERFLOW | FE_INEXACT;
    }
  return 0;
}
static inline int T(call_long_nofenv) (const struct fun *f, struct T(args) a,
					int r, struct RT(ret) * p,
					RT(float) ygot, int exgot)
{
  if (r != FE_TONEAREST)
    fesetround (r);
  RT(double) yl = T(call_long) (f, a);
  p->y = (RT(float)) yl;
  if (r != FE_TONEAREST)
    fesetround (FE_TONEAREST);
  if (RT(isok_nofenv) (ygot, p->y))
    return 1;
  RT (set_long_reference) (yl, p);
  return 0;
}

/* There are nan input args and all quiet.  */
static inline int T(qnanpropagation) (struct T(args) a)
{
  return T(reduce) (a, isnan, ||) && !T(reduce) (a, RT(issignaling), ||);
}
static inline RT(float) T(sum) (struct T(args) a)
{
  return T(reduce) (a, , +);
}

/* returns 1 if the got result is ok.  */
static inline int T(call_mpfr_fix) (const struct fun *f, struct T(args) a,
				     int r_fenv, struct RT(ret) * p,
				     RT(float) ygot, int exgot)
{
#if USE_MPFR
  int t, t2, t_high;
  mpfr_rnd_t r = rmap (r_fenv);
  MPFR_DECL_INIT(my, RT(prec_mpfr));
  MPFR_DECL_INIT(mr, RT(prec));
  MPFR_DECL_INIT(me, RT(prec_mpfr));
  mpfr_clear_flags ();
  t = T(call_mpfr) (my, f, a, r);
  t_high = t;
  /* Double rounding.  */
  t2 = mpfr_set (mr, my, r);
  if (t2)
    t = t2;
  mpfr_set_emin (RT(emin));
  mpfr_set_emax (RT(emax));
  t = mpfr_check_range (mr, t, r);
  t = mpfr_subnormalize (mr, t, r);
  mpfr_set_emax (MPFR_EMAX_DEFAULT);
  mpfr_set_emin (MPFR_EMIN_DEFAULT);
  p->y = mpfr_get_d (mr, r);
  p->ref_finite = mpfr_number_p (my);
  p->ex = t ? FE_INEXACT : 0;
  p->ex_may = FE_INEXACT;
  if (mpfr_underflow_p () && (p->ex & FE_INEXACT))
    /* TODO: handle before and after rounding uflow cases.  */
    p->ex |= FE_UNDERFLOW;
  if (mpfr_overflow_p ())
    p->ex |= FE_OVERFLOW | FE_INEXACT;
  if (mpfr_divby0_p ())
    p->ex |= FE_DIVBYZERO;
  //if (mpfr_erangeflag_p ())
  //  p->ex |= FE_INVALID;
  if (!mpfr_nanflag_p () && RT(isok) (ygot, exgot, p->y, p->ex, p->ex_may))
    return 1;
  if (mpfr_nanflag_p () && !T(qnanpropagation) (a))
    p->ex |= FE_INVALID;
  p->ulpexp = RT (ulpscale_mpfr) (my, t_high);
  if (!isfinite (p->y))
    {
      p->tail = 0;
      if (isnan (p->y))
	{
	  /* If an input was nan keep its sign.  */
	  p->y = T(sum) (a);
	  if (!isnan (p->y))
	    p->y = (p->y - p->y) / (p->y - p->y);
	  return RT(isok) (ygot, exgot, p->y, p->ex, p->ex_may);
	}
      if (!p->ref_finite)
	return 0;
      /* Keep a finite tail when the target result overflows.  */
      mpfr_set_si_2exp (mr, signbit (p->y) ? -1 : 1, RT (emax), MPFR_RNDN);
    }
  mpfr_sub (me, my, mr, MPFR_RNDN);
  mpfr_mul_2si (me, me, -p->ulpexp, MPFR_RNDN);
  p->tail = mpfr_get_d (me, MPFR_RNDN);
  return 0;
#else
  abort ();
#endif
}

static int T(cmp) (const struct fun *f, struct gen *gen,
		     const struct conf *conf)
{
  double maxerr = 0;
  uint64_t cnt = 0;
  uint64_t cnt1 = 0;
  uint64_t cnt2 = 0;
  uint64_t cntfail = 0;
  int r = conf->r;
  int use_mpfr = conf->mpfr;
  int fenv = conf->fenv;

  for (;;)
    {
      struct RT(ret) want;
      struct T(args) a = T(next) (gen);
      int exgot;
      int exgot2;
      RT(float) ygot;
      RT(float) ygot2;
      int fail = 0;
      if (fenv)
	T (call_fenv) (f, a, r, &ygot, &exgot, conf);
      else
	T (call_nofenv) (f, a, r, &ygot, &exgot, conf);
      if (f->twice) {
	secondcall = 1;
	if (fenv)
	  T (call_fenv) (f, a, r, &ygot2, &exgot2, conf);
	else
	  T (call_nofenv) (f, a, r, &ygot2, &exgot2, conf);
	secondcall = 0;
	if (RT(asuint) (ygot) != RT(asuint) (ygot2))
	  {
	    fail = 1;
	    cntfail++;
	    T(printcall) (f, a);
	    printf (" got %a then %a for same input\n", ygot, ygot2);
	  }
      }
      cnt++;
      int ok = use_mpfr
		 ? T(call_mpfr_fix) (f, a, r, &want, ygot, exgot)
		 : (fenv ? T(call_long_fenv) (f, a, r, &want, ygot, exgot)
			 : T(call_long_nofenv) (f, a, r, &want, ygot, exgot));
      if (!ok)
	{
	  int print = 0;
	  double err = RT (ulperr) (ygot, &want, r, conf->ignore_zero_sign);
	  double abserr = fabs (err);
	  // TODO: count errors below accuracy limit.
	  if (abserr > 0)
	    cnt1++;
	  if (abserr > 1)
	    cnt2++;
	  if (abserr > conf->errlim)
	    {
	      print = 1;
	      if (!fail)
		{
		  fail = 1;
		  cntfail++;
		}
	    }
	  if (abserr > maxerr)
	    {
	      maxerr = abserr;
	      if (!conf->quiet && abserr > conf->softlim)
		print = 1;
	    }
	  if (print)
	    {
	      T(printcall) (f, a);
	      // TODO: inf ulp handling
	      printf (" got %a want %a %+g ulp err %g\n", ygot, want.y,
		      want.tail, err);
	    }
	  int diff = fenv ? exgot ^ want.ex : 0;
	  if (fenv && (diff & ~want.ex_may))
	    {
	      if (!fail)
		{
		  fail = 1;
		  cntfail++;
		}
	      T(printcall) (f, a);
	      printf (" is %a %+g ulp, got except 0x%0x", want.y, want.tail,
		      exgot);
	      if (diff & exgot)
		printf (" wrongly set: 0x%x", diff & exgot);
	      if (diff & ~exgot)
		printf (" wrongly clear: 0x%x", diff & ~exgot);
	      putchar ('\n');
	    }
	}
      if (cnt >= conf->n)
	break;
      if (!conf->quiet && cnt % 0x100000 == 0)
	printf ("progress: %6.3f%% cnt %llu cnt1 %llu cnt2 %llu cntfail %llu "
		"maxerr %g\n",
		100.0 * cnt / conf->n, (unsigned long long) cnt,
		(unsigned long long) cnt1, (unsigned long long) cnt2,
		(unsigned long long) cntfail, maxerr);
    }
  double cc = cnt;
  if (cntfail)
    printf ("FAIL ");
  else
    printf ("PASS ");
  T(printgen) (f, gen);
  printf (" round %c errlim %g maxerr %g %s cnt %llu cnt1 %llu %g%% cnt2 %llu "
	  "%g%% cntfail %llu %g%%\n",
	  conf->rc, conf->errlim,
	  maxerr, conf->r == FE_TONEAREST ? "+0.5" : "+1.0",
	  (unsigned long long) cnt,
	  (unsigned long long) cnt1, 100.0 * cnt1 / cc,
	  (unsigned long long) cnt2, 100.0 * cnt2 / cc,
	  (unsigned long long) cntfail, 100.0 * cntfail / cc);
  return !!cntfail;
}
