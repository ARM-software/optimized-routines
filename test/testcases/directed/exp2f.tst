; exp2f.tst - Directed test cases for exp2f
;
; Copyright (C) 2017, ARM Limited, All Rights Reserved
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

func=exp2f op1=7fc00001 result=7fc00001 errno=0
func=exp2f op1=ffc00001 result=7fc00001 errno=0
func=exp2f op1=7f800001 result=7fc00001 errno=0 status=i
func=exp2f op1=ff800001 result=7fc00001 errno=0 status=i
func=exp2f op1=7f800000 result=7f800000 errno=0
func=exp2f op1=7f7fffff result=7f800000 errno=ERANGE status=ox
func=exp2f op1=ff800000 result=00000000 errno=0
func=exp2f op1=ff7fffff result=00000000 errno=ERANGE status=ux
func=exp2f op1=00000000 result=3f800000 errno=0
func=exp2f op1=80000000 result=3f800000 errno=0
func=exp2f op1=42fa0001 result=7e00002c.5c8 errno=0
func=exp2f op1=42ffffff result=7f7fffa7.470 errno=0
func=exp2f op1=43000000 result=7f800000 errno=ERANGE status=ox
func=exp2f op1=43000001 result=7f800000 errno=ERANGE status=ox
func=exp2f op1=c2fa0001 result=00ffffa7.470 errno=0
func=exp2f op1=c2fc0000 result=00800000 errno=0
func=exp2f op1=c2fc0001 result=007fffd3.a38 errno=0 status=ux
func=exp2f op1=c3150000 result=00000001 errno=0
func=exp2f op1=c3158000 result=00000000.800 errno=ERANGE status=ux
func=exp2f op1=c3165432 result=00000000.4bd errno=ERANGE status=ux
