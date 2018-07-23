; sin.tst - Directed test cases for sine
;
; Copyright (c) 1999-2015, Arm Limited.
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

func=sin op1=7ff80000.00000001 result=7ff80000.00000001 errno=0
func=sin op1=fff80000.00000001 result=7ff80000.00000001 errno=0
func=sin op1=7ff00000.00000001 result=7ff80000.00000001 errno=0 status=i
func=sin op1=fff00000.00000001 result=7ff80000.00000001 errno=0 status=i
func=sin op1=7ff00000.00000000 result=7ff80000.00000001 errno=EDOM status=i
func=sin op1=fff00000.00000000 result=7ff80000.00000001 errno=EDOM status=i
func=sin op1=00000000.00000000 result=00000000.00000000 errno=0
func=sin op1=80000000.00000000 result=80000000.00000000 errno=0
func=sin op1=00000000.00000001 result=00000000.00000001 errno=0 status=x maybestatus=u
func=sin op1=80000000.00000001 result=80000000.00000001 errno=0 status=x maybestatus=u
