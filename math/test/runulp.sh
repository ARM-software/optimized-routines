#!/bin/bash

# ULP error check script.
#
# Copyright (c) 2019-2024, Arm Limited.
# SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception

#set -x
set -eu

# cd to bin directory.
cd "${0%/*}"

rmodes='n u d z'
#rmodes=n
flags="${ULPFLAGS:--q}"
emu="$@"

FAIL=0
PASS=0

run_ulp() {
    routine=$1
    shift
    limits_file=$1
    shift
    [ $r == "n" ] || limits_file=${limits_file}_nn
    L=$(grep "^$routine " $limits_file | awk '{print $2}')
    [ -n "$L" ] || { echo ERROR: Could not determine ULP limit for $routine in $limits_file && false; }
    extra_flags="-e $L"
    if grep -q "^$routine$" $DISABLE_FENV; then extra_flags="$extra_flags -f"; fi
    $emu ./ulp $flags $extra_flags "$@" && PASS=$((PASS+1)) || FAIL=$((FAIL+1))
}

t() {
	run_ulp $1 ${GEN_LIMITS} -r $r "$@"
}

t_arch() {
	# Hard-coded flags for arch-specific routines:
	# -r n : arch routines only have to support round-to-nearest
	# -z   : arch routines are permitted to discard sign of zero
	extra_flags="-r n -z"
	run_ulp $1 ${ARCH_LIMITS} ${extra_flags} "$@"
}

check() {
	$emu ./ulp -f -q "$@" >/dev/null
}

for r in $rmodes
do
t exp  0 0xffff000000000000 10000
t exp  0x1p-6     0x1p6     40000
t exp -0x1p-6    -0x1p6     40000
t exp  633.3      733.3     10000
t exp -633.3     -777.3     10000

t exp2  0 0xffff000000000000 10000
t exp2  0x1p-6     0x1p6     40000
t exp2 -0x1p-6    -0x1p6     40000
t exp2  633.3      733.3     10000
t exp2 -633.3     -777.3     10000

t log  0 0xffff000000000000 10000
t log  0x1p-4    0x1p4      40000
t log  0         inf        40000

t log2  0 0xffff000000000000 10000
t log2  0x1p-4    0x1p4      40000
t log2  0         inf        40000

t pow  0.5  2.0  x  0  inf 20000
t pow -0.5 -2.0  x  0  inf 20000
t pow  0.5  2.0  x -0 -inf 20000
t pow -0.5 -2.0  x -0 -inf 20000
t pow  0.5  2.0  x  0x1p-10  0x1p10  40000
t pow  0.5  2.0  x -0x1p-10 -0x1p10  40000
t pow  0    inf  x    0.5      2.0   80000
t pow  0    inf  x   -0.5     -2.0   80000
t pow  0x1.fp-1   0x1.08p0  x  0x1p8 0x1p17  80000
t pow  0x1.fp-1   0x1.08p0  x -0x1p8 -0x1p17 80000
t pow  0         0x1p-1000  x  0 1.0 50000
t pow  0x1p1000        inf  x  0 1.0 50000
t pow  0x1.ffffffffffff0p-1  0x1.0000000000008p0 x 0x1p60 0x1p68 50000
t pow  0x1.ffffffffff000p-1  0x1p0 x 0x1p50 0x1p52 50000
t pow -0x1.ffffffffff000p-1 -0x1p0 x 0x1p50 0x1p52 50000

if [ "$WANT_EXP10_TESTS" = 1 ]; then
t exp10   0                   0x1p-47             5000
t exp10  -0                  -0x1p-47             5000
t exp10   0x1p-47             1                   50000
t exp10  -0x1p-47            -1                   50000
t exp10   1                   0x1.34413509f79ffp8 50000
t exp10  -1                  -0x1.434e6420f4374p8 50000
t exp10  0x1.34413509f79ffp8  inf                 5000
t exp10 -0x1.434e6420f4374p8 -inf                 5000
fi # WANT_EXP10_TESTS

t erf  0 0xffff000000000000 10000
t erf  0x1p-1022  0x1p-26   40000
t erf  -0x1p-1022 -0x1p-26  40000
t erf  0x1p-26    0x1p3     40000
t erf  -0x1p-26  -0x1p3     40000
t erf  0         inf        40000

t expf  0    0xffff0000    10000
t expf  0x1p-14   0x1p8    50000
t expf -0x1p-14  -0x1p8    50000

t exp2f  0    0xffff0000   10000
t exp2f  0x1p-14   0x1p8   50000
t exp2f -0x1p-14  -0x1p8   50000

t logf  0    0xffff0000    10000
t logf  0x1p-4    0x1p4    50000
t logf  0         inf      50000

t log2f  0    0xffff0000   10000
t log2f  0x1p-4    0x1p4   50000
t log2f  0         inf     50000

t sinf  0    0xffff0000    10000
t sinf  0x1p-14  0x1p54    50000
t sinf -0x1p-14 -0x1p54    50000

t cosf  0    0xffff0000    10000
t cosf  0x1p-14  0x1p54    50000
t cosf -0x1p-14 -0x1p54    50000

t sincosf_sinf  0    0xffff0000    10000
t sincosf_sinf  0x1p-14  0x1p54    50000
t sincosf_sinf -0x1p-14 -0x1p54    50000

t sincosf_cosf  0    0xffff0000    10000
t sincosf_cosf  0x1p-14  0x1p54    50000
t sincosf_cosf -0x1p-14 -0x1p54    50000

t powf  0x1p-1   0x1p1  x  0x1p-7 0x1p7   50000
t powf  0x1p-1   0x1p1  x -0x1p-7 -0x1p7  50000
t powf  0x1p-70 0x1p70  x  0x1p-1 0x1p1   50000
t powf  0x1p-70 0x1p70  x  -0x1p-1 -0x1p1 50000
t powf  0x1.ep-1 0x1.1p0 x  0x1p8 0x1p14  50000
t powf  0x1.ep-1 0x1.1p0 x -0x1p8 -0x1p14 50000

t erff  0      0xffff0000 10000
t erff  0x1p-127  0x1p-26 40000
t erff -0x1p-127 -0x1p-26 40000
t erff  0x1p-26   0x1p3   40000
t erff -0x1p-26  -0x1p3   40000
t erff  0         inf     40000

done

# vector functions

if [ "$WANT_SIMD_TESTS" = 1 ]; then

r='n'
flags="${ULPFLAGS:--q}"

range_exp='
  0 0xffff000000000000 10000
  0x1p-6     0x1p6     400000
 -0x1p-6    -0x1p6     400000
  633.3      733.3     10000
 -633.3     -777.3     10000
'

range_log='
  0 0xffff000000000000 10000
  0x1p-4     0x1p4     400000
  0          inf       400000
'

range_pow='
 0x1p-1   0x1p1  x  0x1p-10 0x1p10   50000
 0x1p-1   0x1p1  x -0x1p-10 -0x1p10  50000
 0x1p-500 0x1p500  x  0x1p-1 0x1p1   50000
 0x1p-500 0x1p500  x  -0x1p-1 -0x1p1 50000
 0x1.ep-1 0x1.1p0 x  0x1p8 0x1p16    50000
 0x1.ep-1 0x1.1p0 x -0x1p8 -0x1p16   50000
'

range_sin='
  0       0x1p23     500000
 -0      -0x1p23     500000
  0x1p23  inf        10000
 -0x1p23 -inf        10000
'
range_cos="$range_sin"

range_expf='
  0    0xffff0000    10000
  0x1p-14   0x1p8    500000
 -0x1p-14  -0x1p8    500000
'

range_expf_1u="$range_expf"
range_exp2f="$range_expf"
range_exp2f_1u="$range_expf"

range_logf='
 0    0xffff0000    10000
 0x1p-4    0x1p4    500000
'

range_sinf='
  0        0x1p20   500000
 -0       -0x1p20   500000
  0x1p20   inf      10000
 -0x1p20  -inf      10000
'
range_cosf="$range_sinf"

range_powf='
 0x1p-1   0x1p1  x  0x1p-7 0x1p7   50000
 0x1p-1   0x1p1  x -0x1p-7 -0x1p7  50000
 0x1p-70 0x1p70  x  0x1p-1 0x1p1   50000
 0x1p-70 0x1p70  x  -0x1p-1 -0x1p1 50000
 0x1.ep-1 0x1.1p0 x  0x1p8 0x1p14  50000
 0x1.ep-1 0x1.1p0 x -0x1p8 -0x1p14 50000
'

while read G F
do
	case "$G" in \#*) continue ;; esac
	eval range="\${range_$G}"
	while read X
	do
		[ -n "$X" ] || continue
		case "$X" in \#*) continue ;; esac
		t_arch $F $X
	done << EOF
$range

EOF
done << EOF
# group symbol run
exp       _ZGVnN2v_exp
log       _ZGVnN2v_log
pow       _ZGVnN2vv_pow
sin       _ZGVnN2v_sin
cos       _ZGVnN2v_cos
expf      _ZGVnN4v_expf
expf_1u   _ZGVnN4v_expf_1u
exp2f     _ZGVnN4v_exp2f
exp2f_1u  _ZGVnN4v_exp2f_1u
logf      _ZGVnN4v_logf
sinf      _ZGVnN4v_sinf
cosf      _ZGVnN4v_cosf
powf      _ZGVnN4vv_powf
EOF

fi # WANT_SIMD_TESTS

[ 0 -eq $FAIL ] || {
	echo "FAILED $FAIL PASSED $PASS"
	exit 1
}
