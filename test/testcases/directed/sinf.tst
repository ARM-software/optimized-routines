; sinf.tst - Directed test cases for SP sine
;
; Copyright (C) 2007-2015, ARM Limited, All Rights Reserved
; SPDX-License-Identifier: Apache-2.0
;
; Licensed under the Apache License, Version 2.0 (the "License"); you may
; not use this file except in compliance with the License.
; You may obtain a copy of the License at
;
; http://www.apache.org/licenses/LICENSE-2.0
;
; Unless required by applicable law or agreed to in writing, software
; distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
; WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
; See the License for the specific language governing permissions and
; limitations under the License.
;
; This file is part of the Optimized Routines project


func=sinf op1=7fc00001 result=7fc00001 errno=0
func=sinf op1=ffc00001 result=7fc00001 errno=0
func=sinf op1=7f800001 result=7fc00001 errno=0 status=i
func=sinf op1=ff800001 result=7fc00001 errno=0 status=i
func=sinf op1=7f800000 result=7fc00001 errno=EDOM status=i
func=sinf op1=ff800000 result=7fc00001 errno=EDOM status=i
func=sinf op1=00000000 result=00000000 errno=0
func=sinf op1=80000000 result=80000000 errno=0
; Directed test for a failure I found while developing mathbench
func=sinf op1=c70d39a1 result=be37fad5.7ed errno=0
; SDCOMP-26094: check sinf in the cases for which the range reducer
; returns values furthest beyond its nominal upper bound of pi/4.
func=sinf op1=46427f1b result=3f352d80.f9b error=0
func=sinf op1=4647e568 result=3f352da9.7be error=0
func=sinf op1=46428bac result=bf352dea.924 error=0
func=sinf op1=4647f1f9 result=bf352e13.146 error=0
func=sinf op1=4647fe8a result=3f352e7c.ac9 error=0
func=sinf op1=45d8d7f1 result=3f35097b.cb0 error=0
func=sinf op1=45d371a4 result=bf350990.102 error=0
func=sinf op1=45ce0b57 result=3f3509a4.554 error=0
func=sinf op1=45d35882 result=3f3509f9.bdb error=0
func=sinf op1=45cdf235 result=bf350a0e.02c error=0
