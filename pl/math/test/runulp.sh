#!/bin/bash

# ULP error check script.
#
# Copyright (c) 2019-2022, Arm Limited.
# SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception

#set -x
set -eu

# cd to bin directory.
cd "${0%/*}"

flags="${ULPFLAGS:--q}"
emu="$@"

# Enable SVE testing
WANT_SVE_MATH=${WANT_SVE_MATH:-0}

FAIL=0
PASS=0

t() {
	$emu ./ulp -e $L $flags "$@" && PASS=$((PASS+1)) || FAIL=$((FAIL+1))
}

check() {
	$emu ./ulp -f -q "$@" #>/dev/null
}

L=0.6
t erff  0      0xffff0000 10000
t erff  0x1p-127  0x1p-26 40000
t erff -0x1p-127 -0x1p-26 40000
t erff  0x1p-26   0x1p3   40000
t erff -0x1p-26  -0x1p3   40000
t erff  0         inf     40000

L=0.30
t log10f  0      0xffff0000 10000
t log10f  0x1p-127  0x1p-26 50000
t log10f  0x1p-26   0x1p3   50000
t log10f  0x1p-4    0x1p4   50000
t log10f  0         inf     50000

L=1.11
t log10  0 0xffff000000000000 10000
t log10  0x1p-4    0x1p4      40000
t log10  0         inf        40000

L=3.56
t erfc  0       0xffff0000   10000
t erfc  0x1p-1022  0x1p-26   40000
t erfc -0x1p-1022 -0x1p-26   40000
t erfc  0x1p-26    0x1p5     40000
t erfc -0x1p-26   -0x1p3     40000
t erfc  0          inf       40000

L=1.5
t erfcf  0      0xffff0000 10000
t erfcf  0x1p-127  0x1p-26 40000
t erfcf -0x1p-127 -0x1p-26 40000
t erfcf  0x1p-26    0x1p5  40000
t erfcf -0x1p-26   -0x1p3  40000
t erfcf  0          inf    40000

L=1.78
t atan2     -10.0      10.0               50000
t atan2      -1.0       1.0               40000
t atan2       0.0       1.0               40000
t atan2       1.0     100.0               40000
t atan2       1e6      1e32               40000
t atan2 0x1p-1022 0x1p-1000 x 0 0x1p-1022 40000
# Regression-test for correct NaN handling
check atan2 0x1.7887a0a717aefp+1017 0x1.7887a0a717aefp+1017 x -nan -nan
check atan2 nan nan x -nan -nan

L=2.4
t atan2f -10.0       10.0  50000
t atan2f  -1.0        1.0  40000
t atan2f   0.0        1.0  40000
t atan2f   1.0      100.0  40000
t atan2f   1e6       1e32  40000

L=2.9
t asinhf        0  0x1p-12  5000
t asinhf  0x1p-12      1.0  50000
t asinhf      1.0   0x1p11  50000
t asinhf   0x1p11  0x1p127  20000

L=1.54
t asinh -0x1p-26 0x1p-26   50000
t asinh  0x1p-26     1.0   40000
t asinh -0x1p-26    -1.0   10000
t asinh      1.0   100.0   40000
t asinh     -1.0  -100.0   10000
t asinh    100.0     inf   50000
t asinh   -100.0    -inf   10000

L=1.26
t log1p    -10.0     10.0  10000
t log1p      0.0  0x1p-23  50000
t log1p  0x1p-23    0.001  50000
t log1p    0.001      1.0  50000
t log1p      0.0 -0x1p-23  50000
t log1p -0x1p-23   -0.001  50000
t log1p   -0.001     -1.0  50000
t log1p     -1.0      inf   5000

L=1.52
t log1pf    -10.0     10.0  10000
t log1pf      0.0  0x1p-23  50000
t log1pf  0x1p-23    0.001  50000
t log1pf    0.001      1.0  50000
t log1pf      0.0 -0x1p-23  50000
t log1pf -0x1p-23   -0.001  50000
t log1pf   -0.001     -1.0  50000
t log1pf     -1.0      inf   5000

L=2.80
t tanf  0      0xffff0000 10000
t tanf  0x1p-127  0x1p-14 50000
t tanf -0x1p-127 -0x1p-14 50000
t tanf  0x1p-14   0.7     50000
t tanf -0x1p-14  -0.7     50000
t tanf  0.7       1.5     50000
t tanf -0.7      -1.5     50000
t tanf  1.5       0x1p17  50000
t tanf -1.5      -0x1p17  50000
t tanf  0x1p17    0x1p54  50000
t tanf -0x1p17   -0x1p54  50000
t tanf  0x1p54    inf     50000
t tanf -0x1p54   -inf     50000

L=2.30
t acoshf 0      1         100
t acoshf 1      2       10000
t acoshf 2      0x1p64 100000
t acoshf 0x1p64 inf    100000
t acoshf -0     -inf    10000

L=2.19
t acosh 0        1       10000
t acosh 1        2       100000
t acosh 2        0x1p511 100000
t acosh 0x1p511  inf     100000
t acosh -0      -inf     10000

L=1.02
t expm1f  0        0x1p-23       1000
t expm1f -0       -0x1p-23       1000
t expm1f  0x1p-23  0x1.644716p6  100000
t expm1f -0x1p-23 -0x1.9bbabcp+6 100000

L=1.76
t sinhf  0              0x1.62e43p+6  100000
t sinhf -0             -0x1.62e43p+6  100000
t sinhf  0x1.62e43p+6   0x1.65a9fap+6 100
t sinhf -0x1.62e43p+6  -0x1.65a9fap+6 100
t sinhf  0x1.65a9fap+6  inf           100
t sinhf -0x1.65a9fap+6 -inf           100

L=1.89
t coshf  0              0x1p-63         100
t coshf  0              0x1.5a92d8p+6   80000
t coshf  0x1.5a92d8p+6  inf             2000
t coshf -0             -0x1p-63         100
t coshf -0             -0x1.5a92d8p+6   80000
t coshf -0x1.5a92d8p+6 -inf             2000

L=1.68
t expm1  0                     0x1p-51              1000
t expm1 -0                    -0x1p-51              1000
t expm1  0x1p-51               0x1.63108c75a1937p+9 100000
t expm1 -0x1p-51              -0x1.740bf7c0d927dp+9 100000
t expm1  0x1.63108c75a1937p+9  inf                  100
t expm1 -0x1.740bf7c0d927dp+9 -inf                  100

L=2.08
t sinh  0                    0x1p-51             100
t sinh -0                   -0x1p-51             100
t sinh  0x1p-51              0x1.62e42fefa39fp+9 100000
t sinh -0x1p-51             -0x1.62e42fefa39fp+9 100000
t sinh  0x1.62e42fefa39fp+9  inf                 1000
t sinh -0x1.62e42fefa39fp+9 -inf                 1000

L=1.43
t cosh  0                     0x1.61da04cbafe44p+9 100000
t cosh -0                    -0x1.61da04cbafe44p+9 100000
t cosh  0x1.61da04cbafe44p+9  0x1p10               1000
t cosh -0x1.61da04cbafe44p+9 -0x1p10               1000
t cosh  0x1p10                inf                  100
t cosh -0x1p10               -inf                  100

L=2.59
t atanhf  0        0x1p-12 500
t atanhf  0x1p-12  1       200000
t atanhf  1        inf     1000
t atanhf -0       -0x1p-12 500
t atanhf -0x1p-12 -1       200000
t atanhf -1       -inf     1000

L=1.03
t cbrtf  0  inf 1000000
t cbrtf -0 -inf 1000000

L=2.09
t tanhf  0              0x1p-23       1000
t tanhf -0             -0x1p-23       1000
t tanhf  0x1p-23        0x1.205966p+3 100000
t tanhf -0x1p-23       -0x1.205966p+3 100000
t tanhf  0x1.205966p+3  inf           100
t tanhf -0x1.205966p+3 -inf           100

# vector functions
flags="${ULPFLAGS:--q}"
runs=
check __s_log10f 1 && runs=1
runv=
check __v_log10f 1 && runv=1
runvn=
check __vn_log10f 1 && runvn=1
runsv=
if [ $WANT_SVE_MATH -eq 1 ]; then
check __sv_cosf 0 && runsv=1
check __sv_cos  0 && runsv=1
check __sv_sinf 0 && runsv=1
check __sv_sin 0 && runsv=1
# No guarantees about powi accuracy, so regression-test for exactness
# w.r.t. the custom reference impl in ulp_wrappers.h
check -q -f -e 0 __sv_powif  0  inf x  0  1000 100000 && runsv=1
check -q -f -e 0 __sv_powif -0 -inf x  0  1000 100000 && runsv=1
check -q -f -e 0 __sv_powif  0  inf x -0 -1000 100000 && runsv=1
check -q -f -e 0 __sv_powif -0 -inf x -0 -1000 100000 && runsv=1
check -q -f -e 0 __sv_powi   0  inf x  0  1000 100000 && runsv=1
check -q -f -e 0 __sv_powi  -0 -inf x  0  1000 100000 && runsv=1
check -q -f -e 0 __sv_powi   0  inf x -0 -1000 100000 && runsv=1
check -q -f -e 0 __sv_powi  -0 -inf x -0 -1000 100000 && runsv=1
fi

range_erfc='
   0       0xffff0000   10000
   0x1p-1022  0x1p-26   40000
  -0x1p-1022 -0x1p-26   40000
   0x1p-26    0x1p5     40000
  -0x1p-26   -0x1p3     40000
   0          inf       40000
'

range_erfcf='
   0      0xffff0000 10000
   0x1p-127  0x1p-26 40000
  -0x1p-127 -0x1p-26 40000
   0x1p-26    0x1p5  40000
  -0x1p-26   -0x1p3  40000
   0          inf    40000
'

range_log10='
  0 0xffff000000000000 10000
  0x1p-4     0x1p4     400000
  0          inf       400000
'

range_log10f='
 0    0xffff0000    10000
 0x1p-4    0x1p4    500000
'

range_erf='
 0      0xffff0000 10000
 0x1p-127  0x1p-26 40000
-0x1p-127 -0x1p-26 40000
 0x1p-26   0x1p3   40000
-0x1p-26  -0x1p3   40000
 0         inf     40000
'

range_erff='
 0      0xffff0000 10000
 0x1p-127  0x1p-26 40000
-0x1p-127 -0x1p-26 40000
 0x1p-26   0x1p3   40000
-0x1p-26  -0x1p3   40000
 0         inf     40000
'

range_atan2='
 -10.0       10.0  50000
  -1.0        1.0  40000
   0.0        1.0  40000
   1.0      100.0  40000
   1e6       1e32  40000
'

range_atan='
 -10.0       10.0  50000
  -1.0        1.0  40000
   0.0        1.0  40000
   1.0      100.0  40000
   1e6       1e32  40000
'

range_atan2f='
 -10.0       10.0  50000
  -1.0        1.0  40000
   0.0        1.0  40000
   1.0      100.0  40000
   1e6       1e32  40000
'

range_atanf='
 -10.0       10.0  50000
  -1.0        1.0  40000
   0.0        1.0  40000
   1.0      100.0  40000
   1e6       1e32  40000
'

range_log1pf='
    -10.0     10.0  10000
      0.0  0x1p-23  30000
  0x1p-23    0.001  50000
    0.001      1.0  50000
      0.0 -0x1p-23  30000
 -0x1p-23   -0.001  30000
   -0.001     -1.0  50000
     -1.0      inf   1000
'

range_asinhf='
        0  0x1p-12  40000
  0x1p-12      1.0  40000
      1.0   0x1p11  40000
   0x1p11      inf  40000
        0 -0x1p-12  20000
 -0x1p-12     -1.0  20000
     -1.0  -0x1p11  20000
  -0x1p11     -inf  20000
'

range_log2f='
     -0.0  -0x1p126  100
 0x1p-149  0x1p-126  4000
 0x1p-126   0x1p-23  50000
  0x1p-23       1.0  50000
      1.0       100  50000
      100       inf  50000
'

range_log2='
     -0.0  -0x1p126  100
 0x1p-149  0x1p-126  4000
 0x1p-126   0x1p-23  50000
  0x1p-23       1.0  50000
      1.0       100  50000
      100       inf  50000
'

range_tanf='
     -0.0  -0x1p126  100
 0x1p-149  0x1p-126  4000
 0x1p-126   0x1p-23  50000
  0x1p-23       0.7  50000
      0.7       1.5  50000
      1.5       100  50000
      100    0x1p17  50000
   0x1p17       inf  50000
'

range_log1p='
    -10.0     10.0  10000
      0.0  0x1p-23  50000
  0x1p-23    0.001  50000
    0.001      1.0  50000
      0.0 -0x1p-23  50000
 -0x1p-23   -0.001  50000
   -0.001     -1.0  50000
     -1.0      inf   5000
'

range_expm1f='
  0        0x1p-23       1000
 -0       -0x1p-23       1000
  0x1p-23  0x1.644716p6  1000000
 -0x1p-23 -0x1.9bbabcp+6 1000000
'

range_sinhf='
  0              0x1.62e43p+6  100000
 -0             -0x1.62e43p+6  100000
  0x1.62e43p+6   0x1.65a9fap+6 100
 -0x1.62e43p+6  -0x1.65a9fap+6 100
  0x1.65a9fap+6  inf           100
 -0x1.65a9fap+6 -inf           100
'

range_coshf='
  0              0x1p-63         100
  0              0x1.5a92d8p+6   80000
  0x1.5a92d8p+6  inf             2000
 -0             -0x1p-63         100
 -0             -0x1.5a92d8p+6   80000
 -0x1.5a92d8p+6 -inf             2000
'

range_expm1='
  0                     0x1p-51              1000
 -0                    -0x1p-51              1000
  0x1p-51               0x1.63108c75a1937p+9 100000
 -0x1p-51              -0x1.740bf7c0d927dp+9 100000
  0x1.63108c75a1937p+9  inf                  100
 -0x1.740bf7c0d927dp+9 -inf                  100
'

range_sinh='
  0                    0x1p-51             100
 -0                   -0x1p-51             100
  0x1p-51              0x1.62e42fefa39fp+9 100000
 -0x1p-51             -0x1.62e42fefa39fp+9 100000
  0x1.62e42fefa39fp+9  inf                 1000
 -0x1.62e42fefa39fp+9 -inf                 1000
'

range_cosh='
  0        0x1.6p9   100000
 -0       -0x1.6p9   100000
  0x1.6p9  inf       1000
 -0x1.6p9 -inf       1000
'

range_atanhf='
  0        0x1p-12 500
  0x1p-12  1       200000
  1        inf     1000
 -0       -0x1p-12 500
 -0x1p-12 -1       200000
 -1       -inf     1000
'

range_cbrtf='
  0  inf 1000000
 -0 -inf 1000000
'

range_asinh='
  0        0x1p-26 50000
  0x1p-26  1       50000
  1        0x1p511 50000
  0x1p511  inf     40000
 -0       -0x1p-26 50000
 -0x1p-26 -1       50000
 -1       -0x1p511 50000
 -0x1p511 -inf     40000
'

range_tanhf='
  0              0x1p-23       1000
 -0             -0x1p-23       1000
  0x1p-23        0x1.205966p+3 100000
 -0x1p-23       -0x1.205966p+3 100000
  0x1.205966p+3  inf           100
 -0x1.205966p+3 -inf           100
'

range_sve_cosf='
 0    0xffff0000    10000
 0x1p-4    0x1p4    500000
'

range_sve_cos='
 0    0xffff0000    10000
 0x1p-4    0x1p4    500000
'

range_sve_sinf='
 0    0xffff0000    10000
 0x1p-4    0x1p4    500000
'

range_sve_sin='
 0    0xffff0000    10000
 0x1p-4    0x1p4    500000
'

range_sve_atanf='
 -10.0       10.0  50000
  -1.0        1.0  40000
   0.0        1.0  40000
   1.0      100.0  40000
   1e6       1e32  40000
'

range_sve_atan='
 -10.0       10.0  50000
  -1.0        1.0  40000
   0.0        1.0  40000
   1.0      100.0  40000
   1e6       1e32  40000
'

range_sve_atan2f='
 -10.0       10.0  50000
  -1.0        1.0  40000
   0.0        1.0  40000
   1.0      100.0  40000
   1e6       1e32  40000
'

range_sve_atan2='
 -10.0       10.0  50000
  -1.0        1.0  40000
   0.0        1.0  40000
   1.0      100.0  40000
   1e6       1e32  40000
'

range_sve_log10='
     -0.0  -0x1p126  100
 0x1p-149  0x1p-126  4000
 0x1p-126   0x1p-23  50000
  0x1p-23       1.0  50000
      1.0       100  50000
      100       inf  50000
'

range_sve_log10f='
     -0.0  -0x1p126  100
 0x1p-149  0x1p-126  4000
 0x1p-126   0x1p-23  50000
  0x1p-23       1.0  50000
      1.0       100  50000
      100       inf  50000
'

range_sve_logf='
     -0.0  -0x1p126  100
 0x1p-149  0x1p-126  4000
 0x1p-126   0x1p-23  50000
  0x1p-23       1.0  50000
      1.0       100  50000
      100       inf  50000
'

range_sve_log='
     -0.0  -0x1p126  100
 0x1p-149  0x1p-126  4000
 0x1p-126   0x1p-23  50000
  0x1p-23       1.0  50000
      1.0       100  50000
      100       inf  50000
'

range_sve_expf='
  0        0x1p-23   40000
  0x1p-23  1         50000
  1        0x1p23    50000
  0x1p23   inf       50000
  -0       -0x1p-23  40000
  -0x1p-23 -1        50000
  -1       -0x1p23   50000
  -0x1p23  -inf      50000
'

range_sve_erff='
  0        0x1p-28   20000
  0x1p-28  1         60000
  1        0x1p28    60000
  0x1p28   inf       20000
  -0       -0x1p-28  20000
  -0x1p-28 -1        60000
  -1       -0x1p28   60000
  -0x1p28  -inf      20000
'

range_sve_erf='
  0        0x1p-28   20000
  0x1p-28  1         60000
  1        0x1p28    60000
  0x1p28   inf       20000
  -0       -0x1p-28  20000
  -0x1p-28 -1        60000
  -1       -0x1p28   60000
  -0x1p28  -inf      20000
'

range_sve_tanf='
     -0.0  -0x1p126  100
 0x1p-149  0x1p-126  4000
 0x1p-126   0x1p-23  50000
  0x1p-23       0.7  50000
      0.7       1.5  50000
      1.5       100  50000
      100    0x1p17  50000
   0x1p17       inf  50000
'

range_sve_erfc='
   0      0xffff0000 10000
   0x1p-127  0x1p-26 40000
  -0x1p-127 -0x1p-26 40000
   0x1p-26    0x1p5  40000
  -0x1p-26   -0x1p3  40000
   0          inf    40000
'

# error limits
L_erfc=3.15
L_erfcf=0.26
L_log10=1.97
L_log10f=2.81
L_erf=1.26
L_erff=0.76
# TODO tighten this once __v_atan2 is fixed
L_atan2=2.9
L_atan=1.78
L_atan2f=2.46
L_atanf=2.5
L_log1pf=1.53
L_asinhf=2.17
L_log2f=2.10
L_log2=2.10
L_tanf=2.7
L_log1p=1.97
L_expm1f=1.02
L_sinhf=1.76
L_coshf=1.89
L_expm1=1.68
L_sinh=2.08
L_cosh=1.43
L_atanhf=2.59
L_cbrtf=1.03
L_asinh=1.54
L_tanhf=2.09

L_sve_cosf=1.57
L_sve_cos=1.61
L_sve_sinf=1.40
L_sve_sin=2.03
L_sve_atanf=2.9
L_sve_atan=1.78
L_sve_atan2f=2.45
L_sve_atan2=1.73
L_sve_log10=1.97
L_sve_log10f=2.82
L_sve_logf=2.85
L_sve_log=1.68
L_sve_expf=1.46
L_sve_erff=0.76
L_sve_erf=1.97
L_sve_tanf=2.7
L_sve_erfc=3.15

while read G F R D A
do
	[ "$R" = 1 ] && { [[ $G != sve_* ]] || [ $WANT_SVE_MATH -eq 1 ]; } || continue
	case "$G" in \#*) continue ;; esac
	eval range="\${range_$G}"
	eval L="\${L_$G}"
	while read X
	do
		[ -n "$X" ] || continue
		# fenv checking is enabled by default, but we almost
		# always want to disable it for vector routines. There
		# are, however, a small number of vector routines in
		# pl/math which are supposed to set fenv correctly
		# when WANT_ERRNO is enabled. A hack is needed to
		# ensure fenv checking is enabled for routines where
		# this is the case. Pass "fenv" as fourth argument to
		# prevent -f being added to the run line when
		# WANT_ERRNO is enabled.
		f="-f"
		if [ $WANT_ERRNO -eq 1 ]; then
			if [ "$D" = "fenv" ]; then
				f=""
			elif [ "$D" = "nofenv" ]; then
				# Need to pass this if you want additional
				# arguments but keep fenv checking disabled.
				f="-f"
			elif [ ! -z "$D" ]; then
				echo "Unrecognised 4th argument: $D"
				exit 1
			fi
		fi
		case "$X" in \#*) continue ;; esac
		t $A $f $F $X
	done << EOF
$range
EOF
done << EOF
# group symbol run

atan   __s_atan        $runs
atan   __v_atan        $runv
atan   __vn_atan       $runvn
atan   _ZGVnN2v_atan   $runvn
atan2 __s_atan2        $runs
atan2 __v_atan2        $runv
atan2 __vn_atan2       $runvn
atan2 _ZGVnN2vv_atan2  $runvn
erf   __s_erf          $runs
erf   __v_erf          $runv
erf   __vn_erf         $runvn
erf   _ZGVnN2v_erf     $runvn
erfc   __s_erfc        $runs
erfc   __v_erfc        $runv
erfc   __vn_erfc       $runvn
erfc   _ZGVnN2v_erfc   $runvn
log10  __s_log10       $runs
log10  __v_log10       $runv
log10  __vn_log10      $runvn
log10  _ZGVnN2v_log10  $runvn
log2   __s_log2        $runs
log2   __v_log2        $runv
log2   __vn_log2       $runvn
log2   _ZGVnN2v_log2   $runvn
expm1  __s_expm1       $runs    fenv
expm1  __v_expm1       $runv    fenv
expm1  __vn_expm1      $runvn   fenv
expm1  _ZGVnN2v_expm1  $runvn   fenv
sinh   __s_sinh        $runs    fenv
sinh   __v_sinh        $runv    fenv
sinh   __vn_sinh       $runvn   fenv
sinh   _ZGVnN2v_sinh   $runvn   fenv
cosh   __s_cosh        $runs    fenv
cosh   __v_cosh        $runv    fenv
cosh   __vn_cosh       $runvn   fenv
cosh   _ZGVnN2v_cosh   $runvn   fenv

atanf  __s_atanf       $runs
atanf  __v_atanf       $runv
atanf  __vn_atanf      $runvn
atanf  _ZGVnN4v_atanf  $runvn
atan2f __s_atan2f       $runs
atan2f __v_atan2f       $runv
atan2f __vn_atan2f      $runvn
atan2f _ZGVnN4vv_atan2f $runvn
erff   __s_erff        $runs
erff   __v_erff        $runv
erff   __vn_erff       $runvn
erff   _ZGVnN4v_erff   $runvn
erfcf  __s_erfcf       $runs
erfcf  __v_erfcf       $runv
erfcf  __vn_erfcf      $runvn
erfcf  _ZGVnN4v_erfcf  $runvn
log10f __s_log10f      $runs
log10f __v_log10f      $runv
log10f __vn_log10f     $runvn
log10f _ZGVnN4v_log10f $runvn
log1pf __s_log1pf      $runs
log1pf __v_log1pf      $runv
log1pf __vn_log1pf     $runvn
log1pf _ZGVnN4v_log1pf $runvn
asinhf __s_asinhf      $runs    fenv
asinhf __v_asinhf      $runv    fenv
asinhf __vn_asinhf     $runvn   fenv
asinhf _ZGVnN4v_asinhf $runvn   fenv
log2f  __s_log2f       $runs
log2f  __v_log2f       $runv
log2f  __vn_log2f      $runvn
log2f  _ZGVnN4v_log2f  $runvn
tanf  __s_tanf         $runs
tanf  __v_tanf         $runv
tanf  __vn_tanf        $runvn
tanf  _ZGVnN4v_tanf    $runvn
log1p  __s_log1p       $runs
log1p  __v_log1p       $runv
log1p  __vn_log1p      $runvn
log1p  _ZGVnN2v_log1p  $runvn
expm1f __s_expm1f      $runs    fenv
expm1f __v_expm1f      $runv    fenv
expm1f __vn_expm1f     $runvn   fenv
expm1f _ZGVnN4v_expm1f $runvn   fenv
sinhf  __s_sinhf       $runs    fenv
sinhf  __v_sinhf       $runv    fenv
sinhf  __vn_sinhf      $runvn   fenv
sinhf  _ZGVnN4v_sinhf  $runvn   fenv
coshf  __s_coshf       $runs    fenv
coshf  __v_coshf       $runv    fenv
coshf  __vn_coshf      $runvn   fenv
coshf  _ZGVnN4v_coshf  $runvn   fenv
atanhf __s_atanhf      $runs    fenv -c 0
atanhf __v_atanhf      $runv    fenv -c 0
atanhf __vn_atanhf     $runvn   fenv -c 0
atanhf _ZGVnN4v_atanhf $runvn   fenv -c 0
cbrtf  __s_cbrtf       $runs    fenv
cbrtf  __v_cbrtf       $runv    fenv
cbrtf  __vn_cbrtf      $runvn   fenv
cbrtf  _ZGVnN4v_cbrtf  $runvn   fenv
asinh  __s_asinh       $runs    fenv
# Test vector asinh 3 times, with control lane < 1, > 1 and special.
#  Ensures the v_sel is choosing the right option in all cases.
asinh  __v_asinh       $runv    fenv -c 0.5
asinh  __vn_asinh      $runvn   fenv -c 0.5
asinh  _ZGVnN2v_asinh  $runvn   fenv -c 0.5
asinh  __v_asinh       $runv    fenv -c 2
asinh  __vn_asinh      $runvn   fenv -c 2
asinh  _ZGVnN2v_asinh  $runvn   fenv -c 2
asinh  __v_asinh       $runv    fenv -c 0x1p600
asinh  __vn_asinh      $runvn   fenv -c 0x1p600
asinh  _ZGVnN2v_asinh  $runvn   fenv -c 0x1p600
tanhf  __s_tanhf       $runs    fenv
tanhf  __v_tanhf       $runv    fenv
tanhf  __vn_tanhf      $runvn   fenv
tanhf  _ZGVnN4v_tanhf  $runvn   fenv

sve_cosf     __sv_cosf         $runsv
sve_cosf     _ZGVsMxv_cosf     $runsv
sve_sinf     __sv_sinf         $runsv
sve_sinf     _ZGVsMxv_sinf     $runsv
sve_atan2f   __sv_atan2f       $runsv
sve_atan2f   _ZGVsMxvv_atan2f  $runsv
sve_atanf    __sv_atanf        $runsv
sve_atanf    _ZGVsMxv_atanf    $runsv
sve_log10f   __sv_log10f       $runsv
sve_log10f   _ZGVsMxv_log10f   $runsv
sve_logf     __sv_logf         $runsv
sve_logf     _ZGVsMxv_logf     $runsv
sve_expf     __sv_expf         $runsv
sve_expf     _ZGVsMxv_expf     $runsv
sve_erff     __sv_erff         $runsv
sve_erff     _ZGVsMxv_erff     $runsv
sve_tanf    __sv_tanf          $runsv
sve_tanf    _ZGVsMxv_tanf      $runsv

sve_cos    __sv_cos        $runsv
sve_cos    _ZGVsMxv_cos    $runsv
sve_sin    __sv_sin        $runsv
sve_sin    _ZGVsMxv_sin    $runsv
sve_atan   __sv_atan       $runsv
sve_atan   _ZGVsMxv_atan   $runsv
sve_atan2  __sv_atan2      $runsv
sve_atan2  _ZGVsMxvv_atan2 $runsv
sve_log10  __sv_log10      $runsv
sve_log10  _ZGVsMxv_log10  $runsv
sve_log    __sv_log        $runsv
sve_log    _ZGVsMxv_log    $runsv
sve_erf    __sv_erf        $runsv
sve_erf    _ZGVsMxv_erf    $runsv
sve_erfc   __sv_erfc       $runsv
sve_erfc   _ZGVsMxv_erfc   $runsv
EOF

[ 0 -eq $FAIL ] || {
	echo "FAILED $FAIL PASSED $PASS"
	exit 1
}
