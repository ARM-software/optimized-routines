#!/bin/bash

# ULP error check script.
#
# Copyright (c) 2019-2022, Arm Limited.
# SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception

#set -x
set -eu

# cd to bin directory.
cd "${0%/*}"

rmodes='n'
flags="${ULPFLAGS:--q}"
emu="$@"

FAIL=0
PASS=0

t() {
	[ $r = "n" ] && Lt=$L || Lt=$Ldir
	$emu ./ulp -r $r -e $Lt $flags "$@" && PASS=$((PASS+1)) || FAIL=$((FAIL+1))
}

check() {
	$emu ./ulp -f -q "$@" #>/dev/null
}

Ldir=0.5
for r in $rmodes
do

L=0.6
Ldir=0.9
t erff  0      0xffff0000 10000
t erff  0x1p-127  0x1p-26 40000
t erff -0x1p-127 -0x1p-26 40000
t erff  0x1p-26   0x1p3   40000
t erff -0x1p-26  -0x1p3   40000
t erff  0         inf     40000

L=0.30
Ldir=
t log10f  0      0xffff0000 10000
t log10f  0x1p-127  0x1p-26 50000
t log10f  0x1p-26   0x1p3   50000
t log10f  0x1p-4    0x1p4   50000
t log10f  0         inf     50000

L=1.15
Ldir=
t log10  0 0xffff000000000000 10000
t log10  0x1p-4    0x1p4      40000
t log10  0         inf        40000

done

# vector functions
Ldir=0.5
r='n'
flags="${ULPFLAGS:--q} -f"
runs=
check __s_log10f 1 && runs=1
runv=
check __v_log10f 1 && runv=1
runvn=
check __vn_log10f 1 && runvn=1

range_log10f='
 0    0xffff0000    10000
 0x1p-4    0x1p4    500000
'
# error limits
L_log10f=2.81

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
log10f __s_log10f      $runs
log10f __v_log10f      $runv
log10f __vn_log10f     $runvn
log10f _ZGVnN4v_log10f $runvn
EOF

[ 0 -eq $FAIL ] || {
	echo "FAILED $FAIL PASSED $PASS"
	exit 1
}
