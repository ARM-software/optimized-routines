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

# Enable SVE testing
WANT_SVE_MATH=${WANT_SVE_MATH:-0}

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

L=1.11
Ldir=
t log10  0 0xffff000000000000 10000
t log10  0x1p-4    0x1p4      40000
t log10  0         inf        40000

L=3.55
t erfc  0       0xffff0000   10000
t erfc  0x1p-1022  0x1p-26   40000
t erfc -0x1p-1022 -0x1p-26   40000
t erfc  0x1p-26    0x1p5     40000
t erfc -0x1p-26   -0x1p3     40000
t erfc  0          inf       40000
Ldir=0.5

L=1.5
t erfcf  0      0xffff0000 10000
t erfcf  0x1p-127  0x1p-26 40000
t erfcf -0x1p-127 -0x1p-26 40000
t erfcf  0x1p-26    0x1p5  40000
t erfcf -0x1p-26   -0x1p3  40000
t erfcf  0          inf    40000

L=1.5
t atan2 -10.0       10.0  50000
t atan2  -1.0        1.0  40000
t atan2   0.0        1.0  40000
t atan2   1.0      100.0  40000
t atan2   1e6       1e32  40000

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

L=1.51
t asinh -0x1p-26 0x1p-26   50000
t asinh  0x1p-26     1.0   40000
t asinh -0x1p-26    -1.0   10000
t asinh      1.0   100.0   40000
t asinh     -1.0  -100.0   10000
t asinh    100.0     inf   50000
t asinh   -100.0    -inf   10000

L=1.18
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
runsv=
if [ $WANT_SVE_MATH -eq 1 ]; then
check __sv_cosf 0 && runsv=1
check __sv_cos  0 && runsv=1
check __sv_sinf 0 && runsv=1
check __sv_sin 0 && runsv=1
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

# error limits
L_erfc=3.11
L_erfcf=0.26
L_log10=1.97
L_log10f=2.81
L_erf=1.26
L_erff=0.76
# TODO tighten this once __v_atan2 is fixed
L_atan2=2.9
L_atan=2.15
L_atan2f=2.46
L_atanf=2.5
L_log1pf=1.53
L_asinhf=2.17
L_log2f=2.10
L_log2=2.09

L_sve_cosf=1.57
L_sve_cos=1.61
L_sve_sinf=1.40
L_sve_sin=1.46
L_sve_atanf=2.9
L_sve_atan=1.7
L_sve_atan2f=2.45
# TODO tighten this once __sv_atan2 is fixed
L_sve_atan2=2.0
L_sve_log10=1.97
L_sve_log10f=2.82
L_sve_logf=2.85
L_sve_log=1.68

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
asinhf __s_asinhf      $runs
asinhf __v_asinhf      $runv
asinhf __vn_asinhf     $runvn
asinhf _ZGVnN4v_asinhf $runvn
log2f  __s_log2f       $runs
log2f  __v_log2f       $runv
log2f  __vn_log2f      $runvn
log2f  _ZGVnN4v_log2f  $runvn

if [ $WANT_SVE_MATH -eq 1 ]; then
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
fi
EOF

[ 0 -eq $FAIL ] || {
	echo "FAILED $FAIL PASSED $PASS"
	exit 1
}
