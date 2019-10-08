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
flags='-q'
emu="$@"

FAIL=0
PASS=0

t() {
	[ $r = "n" ] && Lt=$L || Lt=$Ldir
	$emu ./ulp -r $r -e $Lt $flags "$@" && PASS=$((PASS+1)) || FAIL=$((FAIL+1))
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

[ 0 -eq $FAIL ] || {
	echo "FAILED $FAIL PASSED $PASS"
	exit 1
}
