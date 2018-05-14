; cosf.tst - Directed test cases for SP cosine
;
; Copyright (c) 2007-2015, Arm Limited.
; SPDX-License-Identifier: Apache-2.0
;
; Licensed under the Apache License, Version 2.0 (the "License");
; you may not use this file except in compliance with the License.
; You may obtain a copy of the License at
;
;     http://www.apache.org/licenses/LICENSE-2.0
;
; Unless required by applicable law or agreed to in writing, software
; distributed under the License is distributed on an "AS IS" BASIS,
; WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
; See the License for the specific language governing permissions and
; limitations under the License.

func=cosf op1=7fc00001 result=7fc00001 errno=0
func=cosf op1=ffc00001 result=7fc00001 errno=0
func=cosf op1=7f800001 result=7fc00001 errno=0 status=i
func=cosf op1=ff800001 result=7fc00001 errno=0 status=i
func=cosf op1=7f800000 result=7fc00001 errno=EDOM status=i
func=cosf op1=ff800000 result=7fc00001 errno=EDOM status=i
func=cosf op1=00000000 result=3f800000 errno=0
func=cosf op1=80000000 result=3f800000 errno=0
; SDCOMP-26094: check cosf in the cases for which the range reducer
; returns values furthest beyond its nominal upper bound of pi/4.
func=cosf op1=46427f1b result=3f34dc5c.565 error=0
func=cosf op1=4647e568 result=3f34dc33.c1f error=0
func=cosf op1=46428bac result=bf34dbf2.8e3 error=0
func=cosf op1=4647f1f9 result=bf34dbc9.f9b error=0
func=cosf op1=4647fe8a result=3f34db60.313 error=0
func=cosf op1=45d8d7f1 result=bf35006a.7fd error=0
func=cosf op1=45d371a4 result=3f350056.39b error=0
func=cosf op1=45ce0b57 result=bf350041.f38 error=0
func=cosf op1=45d35882 result=bf34ffec.868 error=0
func=cosf op1=45cdf235 result=3f34ffd8.404 error=0
