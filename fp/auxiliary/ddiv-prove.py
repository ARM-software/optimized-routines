#!/usr/bin/env python3

'''Auxiliary tool that proves the error bounds for fp/at32/ddiv.S .

This tool contains code to calculate the table of 8-bit reciprocal
approximations used as input to the Newton-Raphson iteration. It can
print the table directly, for inclusion in the source file, with the
`--print-table` option. Otherwise it calculates the same table entries
on demand and uses them in its other modes.

The calculations() function contains a Python translation of the
numerical algorithm in ddiv.S, starting from an 8-bit table entry,
computing three Newton-Raphson iterations to expand it to a 64-bit
reciprocal of the input denominator, multiplying by the input
numerator to produce an approximate quotient, and normalizing if
necessary to put the leading bit in an appropriate place.

With the `--eval` option, this tool will run the evaluation for a pair
of specific input mantissas. These should be provided in the form they
have at the initial `debug64` macro calls in ddiv.S: each should be a
64-bit word with the top bit set and the next 52 bits being the
fraction field of the input double-precision number. The tool will
calculate the normalized approximate quotient ('nquot'), and print out
all the intermediate results. This allows them to be checked against
the results printed by the debug statements in arm_fp_ddiv itself, if
that function is built with -DDIAGNOSTICS, to confirm that the Python
reference code here matches the assembly language.

As well as printing all the same values printed by the ddiv.S
diagnostics, `--eval` will also print some additional values with
capitalized names, which are the true values that the approximations
are supposed to match closely. For example, 'recip32' is the 32-bit
approximate reciprocal, and 'Recip32' is the _true_ reciprocal scaled
by a power of 2 that makes it match (printed as a hex number with some
fractional hex digits after the point) In particular, the final
normalized quotient approximation 'nquot' is accompanied by 'NQuot',
the true quotient of the two inputs (again with fractional hex
digits). This allows a quick eyeball check that each reciprocal or
quotient approximation roughly matches the value it should.

With the `--find-error` or `--prove-error` options, this tool will
instead invoke calculations() in a mode where instead of actually
calculating anything it uses Gappa and Rocq to derive or prove an
error bound on the overall calculation of the approximate reciprocal.
Each of these must invoke Gappa multiple times, for every combination
of a lookup-table entry (128 choices) and which of the two inputs is
larger (2 choices).

`--find-error` will run Gappa by itself (without invoking Rocq on its
output), and the input files it constructs will use the query
expression

  nquot - NQuot in ?

which asks Gappa to _find_ an interval that the approximation error
lies in. (nquot - NQuot is the absolute difference between the
approximate and true quotients, measured in units in the last place of
the 64-bit integer that nquot is stored as.) Then this script will
identify the largest error in any of its runs, and print it.

`--prove-error ULPS` expects ULPs to be a negative number (though it
can be fractional), e.g. `--prove-error -63.9`. It will write a very
similar set of Gappa input files except that the query expression asks
Gappa to prove

  nquot - NQuot in [ULPS, 0]

The output will be fed to Rocq for cross-checking, so that this mode
should not terminate successfully unless Rocq is happy with the
end-to-end proof that the approximate and true quotients differ by at
most that many ULPs.

To run this tool in the Gappa-using modes, you'll need Gappa itself
installed, plus the Rocq Prover, and also Gappa's helper library of
prewritten Rocq inputs. On Ubuntu 24.04, you can install all of this
using the command

  apt install gappa libcoq-gappa coq

(At the time of writing this, Ubuntu's version of Rocq is still called
`coq`, dating from before the 2025 name change.)

'''

import argparse
import contextlib
import io
import math
import os
import re
import subprocess
import sys
import tempfile

from fractions import Fraction

def calculations(cls, context, a, b, recip08, a_bigger):
    '''Reference implementation of the approximate reciprocal calculation.

    The input `cls` is a class object (not a class instance) which
    supports the common API of `GappaExpression` and `Rational` below,
    implementing some overloaded operators, `floor` and `ceil`
    methods, and class methods for constructing constants and
    variables and identifying things to print.

    `context` is an auxiliary object passed to `cls.define`. (In Gappa
    output mode, it's the file handle to write Gappa variable
    definitions to.)

    `a`, `b` and `recip08` are the inputs to the calculation. Each one
    is an instance of the class `cls`. `a` and `b` represent the
    mantissas of the input numerator and denominator respectively,
    with their leading 1 bit, and scaled by a power of 2 so that they
    are in the range [2^63,2^64). `recip08` is the lookup-table entry
    for the 8-bit reciprocal of the top 8 bits of `b`, as calculated
    by the `approx_reciprocal` function.

    `a_bigger` is a boolean, and indicates which of the inputs is
    expected to be greater. If it is False, then the denominator b is
    expected to be bigger than the numerator a, and the quotient will
    be shifted left by one bit during the calculation.
    '''

    # Define a local one-letter alias for the class method that makes
    # a constant power of 2, so that we can write, e.g., T(48) to
    # construct an instance of `cls` representing 2^48.
    T = cls.power_of_2

    # Produce diagnostic messages (in --eval mode) showing the three
    # inputs to the calculation.
    cls.debug('a', a)
    cls.debug('b', b)
    cls.debug('recip08', recip08)

    # The true reciprocal of b, at the same scale as recip08.
    Recip08 = cls.define(context, 'Recip08', T(71)/b)

    # First Newton-Raphson iteration, using only the top 16 bits of b.
    # We shift right by 14 bits, so that the output is in the range
    # [2^16,2^17).
    recip16 = cls.define(
        context, 'recip16',
        ((recip08 * (T(24) - recip08 * (b / T(48)).floor())) / T(14)).floor())
    # A correspondingly scaled version of the true reciprocal of b.
    Recip16 = cls.define(context, 'Recip16', T(80)/b)

    # Second Newton-Raphson iteration, using the top 31 bits of b.
    recip32 = cls.define(
        context, 'recip32',
        ((recip16 * (T(48) - recip16 * (b / T(33)).floor())) / T(32)).floor())
    # A correspondingly scaled version of the true reciprocal of b.
    Recip32 = cls.define(context, 'Recip32', T(95)/b)

    # Third and last Newton-Raphson iteration, using all of b.
    #
    # Twice in this iteration we must multiply a 64-bit value by the
    # 32-bit value recip32.
    #
    # In the second case this is done in the obvious way, by writing:
    # (recip32 * [value] / T(32)).floor(), simply dividing the full
    # 96-bit product by 2^32 and throwing away the fractional part.
    #
    # But the first of these multiplications is negated after we do
    # it. In the assembly language this is done using one's complement
    # (MVN) rather than two's complement (subtraction from zero), to
    # keep the property that the output is reliably an underestimate
    # of the input.
    #
    # A one's complement negation of a 64-bit value, if bitwise
    # operators aren't available, is most obviously written as
    # (2^64-1) minus the value. So the innermost subexpression would
    # be naturally written as
    #
    #   (T(64) - T(0)) - ((recip32 * b) / T(32)).floor()
    #
    # which could also be viewed (by rearrangement) as one of
    #
    #   T(64) - (((recip32 * b) / T(32)).floor() + T(0))
    #   T(64) - (((recip32 * b) / T(32) + T(0)).floor())
    #
    # In less awkward notation, those expressions would be respectively
    #
    #   (2^64-1) - floor(recip32 * b / 2^32)
    #   2^64 - (floor(recip32 * b / 2^32) + 1)
    #   2^64 - (floor(recip32 * b / 2^32 + 1))
    #
    # Unfortunately, Gappa has trouble proving anything about those
    # expressions, and I wasn't able to find a way to write a hint
    # that would help it. So I used the fact that, for integers x/y,
    #
    #   floor(x/y) + 1 = ceil( (x+1) / y )
    #
    # and I've written the iteration below in terms of the ceil
    # expression on the right-hand side, which Gappa _can_ prove
    # things about.
    #
    # The equivalence between the floor and ceil expressions is easy
    # to see (for a human) by considering the cases where x/y is or is
    # not an integer, but unfortunately, Gappa can't prove it, even
    # given a hint.
    #
    # To add confidence in the correctness of this rewrite, the
    # auxiliary program ddiv-diagnostics.c includes a case where
    # (recip32 * b / 2^32) is an integer, so that you can see that the
    # output recip64 matches between this code and the assembly
    # language, whether or not that is true.
    recip64 = cls.define(
        context, 'recip64',
        (recip32 * (T(64) - ((recip32 * b + T(0)) / T(32)).ceil()) / T(32)).floor())
    # Scaled version of the true reciprocal, to match recip64.
    Recip64 = cls.define(context, 'Recip64', T(126)/b)

    # Calculate the initial approximate quotient, by multiplying a by
    # recip64.
    #
    # The obvious approach (in a 32-bit ISA) would be to divide the
    # 64-bit input a into two halves ah,al; divide the other input
    # recip64 into two halves rh,rl similarly; perform four 64-bit
    # multiplications between those halves; recombine into a 128-bit
    # result.
    #
    # But we're keeping only the top 64 bits of that result, and
    # therefore, we save time by completely skipping the
    # multiplication of the low halves al,rl. We also throw away the
    # bottom half of each of the two middle-sized products ah*rl and
    # al*rh, rather than bothering to add them together and propagate
    # a carry.
    #
    # Again this must be described to Gappa in a careful manner, to
    # allow it to understand it well enough to prove things about.
    #
    # First define the four variables giving the halves of the inputs.
    ah = cls.define(context, 'ah', (a / T(32)).floor())
    al = cls.define(context, 'al', a / T(32) - ah)
    rh = cls.define(context, 'rh', (recip64 / T(32)).floor())
    rl = cls.define(context, 'rl', recip64 / T(32) - rh)
    # `fullquot` is the full 128-bit quotient, scaled down so that the
    # top 64 bits are the integer part and the bottom 64 bits are the
    # fractional part.
    fullquot = cls.define(context, 'fullquot', a * recip64 / T(64))
    # `mul_error` is the difference between `fullquot` and the
    # quotient we actually compute. It's composed of three terms, all
    # negative: al*rl, and the fractional parts of ah*rl and al*rh.
    #
    # This variable isn't used in the rest of the calculation, but
    # needs to exist so that it can be mentioned in a hint to Gappa's
    # prover.
    mul_error = cls.define(
        context, 'mul_error',
        -al*rl - (ah*rl - (ah*rl).floor()) - (al*rh - (al*rh).floor()))
    # `quot` _is_ the quotient we actually compute, described directly
    # in terms of the pieces of the 128-bit product a*r that we _do_ keep.
    quot = cls.define(context, 'quot',
                      ah*rh + (ah*rl).floor() + (al*rh).floor())
    # `Quot` is the true quotient a/b, scaled to match `quot`.
    Quot = cls.define(context, 'Quot', a * Recip64 / T(64))

    # Now make the normalized quotient, with its leading 1 bit in the
    # same place regardless of whether a or b was bigger. `quot` is an
    # integer, so we simply multiply by 2^0 or 2^1 as appropriate.
    renorm = 0 if a_bigger else 1
    nquot = cls.define(context, 'nquot', quot * T(renorm))
    # `NQuot` is the true quotient, scaled to match `nquot`. In other
    # words, the difference (nquot - NQuot) is precisely the overall
    # approximation error that we are trying to establish a bound on.
    NQuot = cls.define(context, 'NQuot', Quot * T(renorm))

class GappaExpression:
    '''Parameter class for calculations(), producing Gappa input syntax.

    When calculations() is run with this class as a parameter, the
    inputs `a`, `b` and `recip08` don't have numeric values. Instead
    they are symbolic expressions representing an unknown, which will
    be written into the Gappa input as simply variable names, so that
    the Gappa query expression can ask for an analysis based on them
    taking any value in a given range.

    An instance of this class has just one field, `text`, which is a
    string containing a valid Gappa expression.
    '''
    def __init__(self, text):
        '''Trivial constructor, only called by methods of this class'''
        self.text = text

    @classmethod
    def power_of_2(cls, exponent):
        '''Make a Gappa constant representing a power of 2.

        We use the C hex float syntax 0x1pN (e.g. 0x1p32, 0x1p-8) to
        represent the value 2^N, with the exception that 2^0 is
        represented as just '1'.
        '''
        return cls("1" if exponent == 0 else f"0x1p{exponent}")

    @classmethod
    def variable(cls, name):
        '''Make a GappaExpression wrapping just a variable name.

        This isn't part of the common API between this class and
        `Rational`. It must only be called by code that knows it's
        dealing with this particular class: the `gappa_input()`
        function, and other methods of this class.
        '''
        return cls(name)

    @classmethod
    def debug(cls, name, value):
        '''Stub implementation of the debug function.

        We can't generate debug output printing the values of the
        variables if the values aren't known.
        '''
        pass

    @classmethod
    def define(cls, context, varname, expression):
        '''Define a new variable name and return a GappaExpression
        denoting its value.

        The variable definition is written to the Gappa output file as
        an assignment statement, and the returned class instance uses
        the newly defined variable by name.
        '''
        print(f"{varname} = {expression.text};", file=context)
        return cls.variable(varname)

    # Arithmetic operations, plus floor() and ceil(), all work by
    # constructing a Gappa expression incorporating the operands as
    # subexpressions. Parentheses are always added, to ensure operator
    # precedence never causes a problem.
    def __add__(self, other):
        return type(self)(f"({self.text} + {other.text})")
    def __sub__(self, other):
        return type(self)(f"({self.text} - {other.text})")
    def __neg__(self):
        return type(self)(f"-({self.text})")
    def __mul__(self, other):
        return type(self)(f"({self.text} * {other.text})")
    def __truediv__(self, other):
        return type(self)(f"({self.text} / {other.text})")
    def floor(self):
        return type(self)(f"floor({self.text})")
    def ceil(self):
        return type(self)(f"ceil({self.text})")

class Rational:
    '''Parameter class for calculations(), for testing purposes.

    When calculations() is run with this class as a parameter, the
    inputs `a`, `b` and `recip08` each wrap a specific numeric value,
    and the calculations are actually performed as specified. The
    `debug` and `define` methods each output a diagnostic printing the
    variable name and its value, with 16 fractional hex digits if the
    value isn't an integer.

    An instance of this class has just one field, `val`, which is a
    fractions.Fraction storing an exact rational number.

    '''
    def __init__(self, n, d=1):
        '''Trivial constructor, only called by methods of this class'''
        self.val = Fraction(n, d)

    @classmethod
    def power_of_2(cls, exponent):
        '''Make a Fraction representing a power of 2.'''
        return cls(Fraction(2)**exponent)

    @classmethod
    def debug(cls, name, value):
        '''Print a diagnostic containing a variable name and a value.'''

        # Normalise the number to be positive, and remember to print a
        # "-" at the front if it was originally negative.
        sign, val = "", value.val
        if val < 0:
            sign, val = "-", -val

        # Take the integer part of the number.
        intval = math.floor(val)
        if val == intval:
            # The number is an integer, so just print the integer
            # value by itself (with the prefixed sign).
            print(f"{name} = {sign}0x{intval:X}")
        else:
            # The number isn't an integer, so find its fractional part
            # by subtracting off `intval`; scale that up by 2^64 and
            # take the integer part again, to get 16 fractional hex
            # digits.
            more_digits = math.floor((val - intval) * 2**64)
            print(f"{name} = {sign}0x{intval:X}.{more_digits:016X}")

    @classmethod
    def define(cls, context, varname, expression):
        '''Print a diagnostic for each new variable the caller defines.

        The returned class instance is just the same one passed in.
        '''
        cls.debug(varname, expression)
        return expression

    # Arithmetic operators on this class just wrap the same operators
    # on fractions.Fraction itself. floor and ceil are implemented
    # using math.floor() and math.ceil(), which already have special
    # cases for fractions.Fraction which don't require a lossy cast to
    # float.
    def __add__(self, other):
        return type(self)(self.val + other.val)
    def __sub__(self, other):
        return type(self)(self.val - other.val)
    def __neg__(self):
        return type(self)(-self.val)
    def __mul__(self, other):
        return type(self)(self.val * other.val)
    def __truediv__(self, other):
        return type(self)(self.val / other.val)
    def floor(self):
        return type(self)(math.floor(self.val))
    def ceil(self):
        return type(self)(math.ceil(self.val))

def approx_reciprocal(topbits):
    '''Calculate an entry in the 8-bit reciprocal lookup table.

    Each entry applies to a range of 2^45 possible input values, all
    with the same top 8 mantissa bits (including the leading 1). The
    table entry is the reciprocal of the midpoint of that range,
    scaled to be in the range [0x80,0x100), and rounded to the nearest
    integer in that range.

    This function is the single definition of the lookup table, used
    by --print-table (so that it matches what's in ddiv.S) and also by
    callers of calculations() so that it matches what the Gappa proof
    uses.
    '''
    assert 0x80 <= topbits < 0x100
    return math.floor(Fraction(0x10000, (2*topbits + 1)) + Fraction(1,2))

def gappa_input(bmin, bmax, a_bigger, errbound):
    '''Generate a Gappa input file and return it as a string.

    The input file will ask Gappa to discover or verify a bound on the
    error in the calculation done in `calculations()`, for a given
    range of input denominators `b` and a given choice of which of `b`
    and the numerator `a` is bigger (and hence whether the quotient
    must be renormalized once it's been calculated).

    `bmin` and `bmax` specify the range of `b` for which this input
    file will compute an error bound. Both bounds are inclusive. The
    two values must agree on their top 8 bits, so that the calculation
    can be based on a single entry from the 8-bit reciprocal lookup
    table. In normal usage they specify the full range of inputs that
    a single table entry applies to, e.g. you might set
    bmin=0xAB00000000000000, bmax=0xABFFFFFFFFFFFFFF. By using a
    smaller range you can attempt to zero in on 'problem' denominators
    which are the limiting factor on how small the error bound can be.

    `a_bigger` is a boolean indicating whether the numerator `a` is
    expected to be greater or less than `b` in this run.

    If `errbound` is a number, then the returned Gappa input will ask
    Gappa to prove that the error is in the range [errbound,0].
    (`errbound` should therefore be negative.) Alternatively,
    `errbound` can be `None`, in which case Gappa will be asked to
    _state_ what range it thinks the error is in, by replacing the
    error interval with a `?` in the query part of the input file.
    '''

    # Range-check bmin and bmax.
    assert 2**63 <= bmin <= bmax < 2**64

    # Determine the lookup table entry we need, and ensure it's consistent.
    topbits = bmin >> 56
    assert topbits == bmax >> 56
    recip = approx_reciprocal(topbits)

    # Begin accumulating the Gappa program in a StringIO.
    fh = io.StringIO()

    # Preamble: define 'floor' and 'ceil' in Gappa terminology, to
    # mean converting to an integer, rounding down or up respectively.
    fh.write('''\
@floor = int<dn>;
@ceil = int<up>;

''')

    # Call calculations(), passing it the class GappaExpression as its
    # parameter, and our StringIO file handle as its context, so that
    # instead of actually doing the calculation it will print a
    # sequence of variable definitions into our output Gappa code,
    # specifying the same calculations symbolically.
    cls = GappaExpression
    calculations(cls, fh, cls.variable('a'), cls.variable('b'),
                 cls.variable('recip08'), a_bigger)

    # Write out the query section of the Gappa input file: after we've
    # defined all our variables, actually ask it to place bounds on a
    # particular expression.
    #
    # The expression we want to bound is (nquot - NQuot). `nquot` is
    # the normalized quotient actually computed by the machine code,
    # in the form of a 64-bit integer. `NQuot` is the true value a/b,
    # scaled appropriately to have its leading bit in the same place,
    # so it's the value `nquot` is approximating. We want to bound the
    # absolute difference between the approximation and the truth.
    #
    # The Gappa query section also puts bounds on the input variables.
    # We specify a range of values of `b` as given by our inputs. We
    # give the full range of values of `a` covering the whole space of
    # 64-bit numbers with the leading bit set, but then constrain it
    # so that either a<b or a>b according to our input `a_bigger`
    # flag. Finally, we set `recip08` to be the lookup table entry we
    # retrieved above, which matches the one the machine code
    # implementation would look up for any denominator in this range.
    ab_constraint = "a/b >= 1" if a_bigger else "a/b <= 1"
    errinterval = "?" if errbound is None else f"[{errbound:.4f},0]"
    fh.write(f'''
{{ b in [0x{bmin:016X},0x{bmax:016X}] /\\
  recip08 = 0x{recip:02X} /\\
  a in [0x8000000000000000, 0xFFFFFFFFFFFFFFFF] /\\ {ab_constraint} ->
  nquot - NQuot in {errinterval} }}
''')

    # Now write out a 'hints' section of the Gappa input file. This
    # gives Gappa a hand with the algebra, by suggesting how it should
    # transform an expression to make a useful deduction possible.

    # Our first three hints allow Gappa to understand the quadratic
    # convergence property of our Newton-Raphson iterations. The basic
    # idea is similar to an example in the Gappa documentation, which
    # also deals with Newton-Raphson based division, but a much
    # simpler case of it implemented in simple fixed point, without
    # all the power-of-2 scaling and discarding of low-order bits that
    # our version does.
    #
    # Their example is to write a hint saying
    #
    # r0 * (2 - d * r0) - R -> (r0 - R) * (r0 - R) * -d;
    #
    # in which d is the value you want the reciprocal of; R is the
    # true reciprocal 1/d; and r0 is the input to a particular
    # iteration. The LHS of the hint is the difference between the
    # output of this Newton-Raphson iteration and the true value R;
    # the RHS is a rearrangement of the same expression, in such a way
    # that you can see that it has two factors of (r0-R), which is the
    # error in the _input_ approximation. This rewrite allows Gappa to
    # establish a bound on the output error obtained by squaring the
    # input error.
    #
    # Each of these three hints has a left-hand side of the form
    # mimicking the calculation of one of our actual iterations, with
    # the truncation operations (floor() and ceil()) missing: Gappa
    # can figure out for itself how things change when the floors and
    # ceils are reinserted. The full LHS takes the difference between
    # that formula and Recip16, Recip32 or Recip64, each of which is
    # an appropriately scaled version of the true reciprocal. The RHS
    # in each case is a rearrangement which includes a squared factor
    # involving the error in the previous approximation.
    #
    # Gappa will check that the two sides of each of these suggested
    # rewrites are equivalent, by reducing both to a canonical form
    # and checking they match. So it can see that each of these
    # rewrites is _correct_. But it needed the hint to tell it that
    # each one would be _useful_.
    fh.write('''
(recip08 * (0x1p24 - recip08 * (b/0x1p48))) / 0x1p14 - Recip16 ->
  (-(recip08 - Recip08) * (recip08 - Recip08) * b / 0x1p62)
  { b <> 0 };

(recip16 * (0x1p48 - recip16 * (b/0x1p33))) / 0x1p32 - Recip32 ->
  (-(recip16 - Recip16) * (recip16 - Recip16) * b / 0x1p65)
  { b <> 0 };

(recip32 * ((0x1p64 - (recip32 * b + 1) / 0x1p32)) / 0x1p32) - Recip64 ->
  (-(recip32 - Recip32) * (recip32 - Recip32) * b / 0x1p64) - recip32/0x1p64
  { b <> 0 };
''')

    # One last hint, to help Gappa work out what happened during the
    # final not-quite-complete 128-bit multiplication of the numerator
    # and the final reciprocal approximation. We defined `quot` to be
    # the parts of the 128-bit product that the machine code actually
    # keeps; `fullquot` to be the value of the 128-bit product if we'd
    # computed it in full; and `mul_error` to be a list of the parts
    # we threw away. By itself Gappa can't spot that our slightly
    # incomplete quotient differs from the true one by only a little
    # bit, but if we explain that it _equals_ the true one plus
    # `mul_error`, it knows it only needs to compute a bound on
    # `mul_error`, and it can see that that's small.
    fh.write('''
quot - Quot -> fullquot - Quot + mul_error { b <> 0 };
''')

    return fh.getvalue()

def outfilename(bmin, bmax, a_bigger, output_dir, extension):
    '''Construct a name for an output file.

    The parameters `bmin`, `bmax` and `a_bigger` have the same
    semantics as they do for `gappa_input`, and are encoded in the
    file's base name. `output_dir` gives the pathname of a directory
    to put the file in, and `extension` gives a file extension.
    '''
    bigger = "a_bigger" if a_bigger else "b_bigger"
    return os.path.join(
        output_dir, f"ddiv_{bmin:016X}_{bmax:016X}_{bigger}.{extension}")

def worst_error(bmin, bmax, a_bigger, output_dir):
    '''Run Gappa to calculate a bound on approximation error.

    All the parameters to this function are passed straight through to
    `gappa_input`, and have the same semantics as documented there.

    This function passes `None` as the error bound, to ask Gappa to
    calculate it instead of proving a suggested one. Then it runs
    Gappa itself with the provided input, and parses the output to
    find the lower bound on the error.

    (The upper bound on the error is always expected to be 0, because
    this division algorithm always underestimates. This function does
    not check the upper interval bound. Therefore after you derive an
    error bound you should always use the proving mode to be sure of
    it.)
    '''

    # Make the Gappa input.
    gappa_this = gappa_input(bmin, bmax, a_bigger, None)

    # Save it to a file.
    gappa_filename = outfilename(bmin, bmax, a_bigger, output_dir, "g")
    with open(gappa_filename, "w") as fh:
        fh.write(gappa_this)

    # Run Gappa on that file and capture its stderr.
    result = subprocess.run(["gappa", gappa_filename],
                            stderr=subprocess.PIPE, check=True)

    # Write Gappa's stderr to another diagnostic file.
    stderr_filename = outfilename(bmin, bmax, a_bigger, output_dir,
                                  "gappa_output")
    with open(stderr_filename, "wb") as fh:
        fh.write(result.stderr)

    # Match Gappa's stderr against a simple regex to extract the error
    # bound, and return it as a Fraction.
    m = re.search(r"{([^,]+)", result.stderr.decode().splitlines()[1])
    e = Fraction(m.group(1))
    return e

def prove_error_bound(bmin, bmax, a_bigger, errbound, output_dir):
    '''Run Gappa to calculate a bound on approximation error.

    `errbound` is expected to be a negative number. This function will
    ask Gappa to prove that the approximation error (for this
    particular section of the input space) is always in the interval
    [errbound,0]: that is, the approximate quotient is never an
    overestimate, and underestimates by at most `errbound` units in
    the last place of the output 64-bit integer.

    All the parameters to this function are passed straight through to
    `gappa_input`, and have the same semantics as documented there.

    This function runs Gappa with the provided input, and makes it
    output Rocq input in turn, which it then passes to `coqc` as an
    independent check on Gappa's proof of the error bound.
    '''

    assert errbound < 0, "Lower error bound should be negative"

    # Make the Gappa input.
    gappa_this = gappa_input(bmin, bmax, a_bigger, errbound)

    # Save it to a file.
    gappa_filename = outfilename(bmin, bmax, a_bigger, output_dir, "g")
    with open(gappa_filename, "w") as fh:
        fh.write(gappa_this)

    # Run Gappa on that file, and ask it to output a Rocq proof.
    rocq_filename = outfilename(bmin, bmax, a_bigger, output_dir, "v")
    result = subprocess.run(
        ["gappa", "-Bcoq"],
        input="".join(gappa_this).encode(),
        stdout=open(rocq_filename, "wb"))
    if result.returncode != 0:
        return "gappa failed"

    # Pass the Rocq proof to Rocq itself, and make sure it's happy with it.
    result = subprocess.run(["coqc", rocq_filename])
    if result.returncode != 0:
        return "coqc failed"

    # Done.
    return None

def main():
    '''Main program: parse arguments and decide what to do.'''

    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)

    # Mutually exclusive group of options telling this script which
    # mode to run in.
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument("--print-table", action='store_true',
                       help="print the table of reciprocals")
    group.add_argument("--find-error", action='store_true',
                       help="find the maximum error")
    group.add_argument("--prove-error", type=Fraction, metavar="ULPS",
                       help="prove a value of the maximum error")
    group.add_argument("--eval", nargs=2, type=lambda s: int(s,0),
                       metavar=("NUM", "DENOM"),
                       help="evaluate a quotient and its intermediate results")

    # Option affecting both of --find-error and --prove-error modes.
    parser.add_argument("--output-dir", metavar="PATH", help="write output "
                        "files to this directory")

    args = parser.parse_args()

    if args.output_dir is None:
        workdir = tempfile.TemporaryDirectory
    else:
        @contextlib.contextmanager
        def workdir():
            os.makedirs(args.output_dir, exist_ok=True)
            yield args.output_dir

    if args.eval is not None:
        # --eval mode: the user provided specific values for the
        # mantissas of the two inputs, and we call `calculations()`
        # with the `Rational` parameter class to perform the
        # calculation and print all the intermediate and final
        # results. This allows those results to be checked against
        # what the machine code version does with the same inputs, to
        # demonstrate that this code and the machine code are
        # implementing the same algorithm.
        a, b = args.eval
        recip = approx_reciprocal(b >> 56)
        calculations(Rational, None, Rational(a), Rational(b),
                     Rational(recip), a>b)

    elif args.find_error:
        # --find-error mode: call `worst_error()` for all combinations
        # of a lookup table entry and the `a_bigger` flag, and take
        # the min of all the resulting lower bounds on the
        # approximation error.
        with workdir() as wd:
            worst = 0
            for topbits in range(0x80, 0x100):
                for a_bigger in [False, True]:
                    err = worst_error(topbits << 56, ((topbits+1) << 56) - 1,
                                      a_bigger, wd)
                    print(f"topbits={topbits:02x} a_bigger={a_bigger}: "
                          f"{err:.4f}")
                    assert err < 0
                    worst = min(worst, err)
        print(f"Worst error = {worst:.4f}")

    elif args.prove_error is not None:
        # --prove-error mode: given an input error bound, call
        # `prove_error_bound()` to try to prove that bound is valid
        # for all combinations of a lookup table entry and the
        # `a_bigger` flag. Returns a success or failure exit code.
        failed = False
        with workdir() as wd:
            for topbits in range(0x80, 0x100):
                for a_bigger in [False, True]:
                    result = prove_error_bound(
                        topbits << 56, ((topbits+1) << 56) - 1,
                        a_bigger, args.prove_error, wd)
                    text = result if result is not None else "ok"
                    print(f"topbits={topbits:02x} a_bigger={a_bigger}: {text}")
                    if result is not None:
                        failed = True
        if failed:
            sys.exit("proof failed")
        else:
            print("proof succeeded!")

    elif args.print_table:
        # --print-table mode: just compute the full lookup table of
        # 8-bit reciprocals, and output it in a form suitable for
        # pasting into ddiv.S.
        linelen = 8
        for topbits in range(0x80, 0x100, linelen):
            recips = list(map(approx_reciprocal,
                              range(topbits, topbits+linelen)))
            print("  .byte", ",".join(f"0x{recip:02X}" for recip in recips))

if __name__ == '__main__':
    main()
