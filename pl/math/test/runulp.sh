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

[ 0 -eq $FAIL ] || {
	echo "FAILED $FAIL PASSED $PASS"
	exit 1
}
