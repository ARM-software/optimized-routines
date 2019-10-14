#!/bin/bash

# ULP error check script.
#
# Copyright (c) 2019, Arm Limited.
# SPDX-License-Identifier: MIT

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

t() {
	[ $r = "n" ] && Lt=$L || Lt=$Ldir
	$emu ./ulp -r $r -e $Lt $flags "$@" && PASS=$((PASS+1)) || FAIL=$((FAIL+1))
}

check() {
	$emu ./ulp -f -q "$@" >/dev/null
}

Ldir=0.5
for r in $rmodes
do
L=0.01
t exp  0 0xffff000000000000 10000
t exp  0x1p-6     0x1p6     40000
t exp -0x1p-6    -0x1p6     40000
t exp  633.3      733.3     10000
t exp -633.3     -777.3     10000

L=0.01
t exp2  0 0xffff000000000000 10000
t exp2  0x1p-6     0x1p6     40000
t exp2 -0x1p-6    -0x1p6     40000
t exp2  633.3      733.3     10000
t exp2 -633.3     -777.3     10000

L=0.05
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

L=0.01
t expf  0    0xffff0000    10000
t expf  0x1p-14   0x1p8    50000
t expf -0x1p-14  -0x1p8    50000

L=0.01
t exp2f  0    0xffff0000   10000
t exp2f  0x1p-14   0x1p8   50000
t exp2f -0x1p-14  -0x1p8   50000

L=0.06
t sinf  0    0xffff0000    10000
t sinf  0x1p-14  0x1p54    50000
t sinf -0x1p-14 -0x1p54    50000

L=0.06
t cosf  0    0xffff0000    10000
t cosf  0x1p-14  0x1p54    50000
t cosf -0x1p-14 -0x1p54    50000

L=0.4
t powf  0x1p-1   0x1p1  x  0x1p-7 0x1p7   50000
t powf  0x1p-1   0x1p1  x -0x1p-7 -0x1p7  50000
t powf  0x1p-70 0x1p70  x  0x1p-1 0x1p1   50000
t powf  0x1p-70 0x1p70  x  -0x1p-1 -0x1p1 50000
t powf  0x1.ep-1 0x1.1p0 x  0x1p8 0x1p14  50000
t powf  0x1.ep-1 0x1.1p0 x -0x1p8 -0x1p14 50000
done

# vector functions
Ldir=0.5
r='n'
flags="${ULPFLAGS:--q} -f"
runv=
check __v_exp 1 && runv=1
runvn=
check __vn_exp 1 && runvn=1

range_exp='
  0 0xffff000000000000 10000
  0x1p-6     0x1p6     400000
 -0x1p-6    -0x1p6     400000
  633.3      733.3     10000
 -633.3     -777.3     10000
'

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
 0    0xffff0000    10000
 0x1p-4    0x1p4    300000
-0x1p-9   -0x1p9    300000
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

# error limits
L_exp=1.9
L_expf=1.49
L_expf_1u=0.4
L_exp2f=1.49
L_exp2f_1u=0.4
L_logf=2.9
L_sinf=1.4
L_cosf=1.4
L_powf=2.1

while read G F R
do
	[ "$R" = 1 ] || continue
	case "$G" in \#*) continue ;; esac
	eval range="\${range_$G}"
	eval L="\${L_$G}"
	while read X
	do
		[ -n "$X" ] || continue
		case "$X" in \#*) continue ;; esac
		t $F $X
	done << EOF
$range
EOF
done << EOF
# group symbol run
exp  __s_exp       1
exp  __v_exp       $runv
exp  __vn_exp      $runvn
exp  _ZGVnN2v_exp  $runvn

expf __s_expf      1
expf __v_expf      $runv
expf __vn_expf     $runvn
expf _ZGVnN4v_expf $runvn

expf_1u __s_expf_1u   1
expf_1u __v_expf_1u   $runv
expf_1u __vn_expf_1u  $runvn

exp2f __s_exp2f      1
exp2f __v_exp2f      $runv
exp2f __vn_exp2f     $runvn
exp2f _ZGVnN4v_exp2f $runvn

exp2f_1u __s_exp2f_1u  1
exp2f_1u __v_exp2f_1u  $runv
exp2f_1u __vn_exp2f_1u $runvn

logf __s_logf      1
logf __v_logf      $runv
logf __vn_logf     $runvn
logf _ZGVnN4v_logf $runvn

sinf __s_sinf      1
sinf __v_sinf      $runv
sinf __vn_sinf     $runvn
sinf _ZGVnN4v_sinf $runvn

cosf __s_cosf      1
cosf __v_cosf      $runv
cosf __vn_cosf     $runvn
cosf _ZGVnN4v_cosf $runvn

powf __s_powf       1
powf __v_powf       $runv
powf __vn_powf      $runvn
powf _ZGVnN4vv_powf $runvn
EOF

[ 0 -eq $FAIL ] || {
	echo "FAILED $FAIL PASSED $PASS"
	exit 1
}
