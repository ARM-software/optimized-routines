; expf.tst - Directed test cases for expf
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

func=expf op1=7fc00001 result=7fc00001 errno=0
func=expf op1=ffc00001 result=7fc00001 errno=0
func=expf op1=7f800001 result=7fc00001 errno=0 status=i
func=expf op1=ff800001 result=7fc00001 errno=0 status=i
func=expf op1=7f800000 result=7f800000 errno=0
func=expf op1=7f7fffff result=7f800000 errno=ERANGE status=ox
func=expf op1=ff800000 result=00000000 errno=0
func=expf op1=ff7fffff result=00000000 errno=ERANGE status=ux
func=expf op1=00000000 result=3f800000 errno=0
func=expf op1=80000000 result=3f800000 errno=0
func=expf op1=42affff8 result=7ef87ed4.e0c errno=0
func=expf op1=42b00008 result=7ef88698.f67 errno=0
func=expf op1=42cffff8 result=7f800000 errno=ERANGE status=ox
func=expf op1=42d00008 result=7f800000 errno=ERANGE status=ox
func=expf op1=c2affff8 result=0041eecc.041 errno=0 status=ux
func=expf op1=c2b00008 result=0041ecbc.95e errno=0 status=ux
func=expf op1=c2cffff8 result=00000000 errno=ERANGE status=ux
func=expf op1=c2d00008 result=00000000 errno=ERANGE status=ux
