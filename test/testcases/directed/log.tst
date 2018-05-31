; Directed test cases for log
;
; Copyright (c) 2018, Arm Limited.
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

func=log op1=7ff80000.00000001 result=7ff80000.00000001 errno=0
func=log op1=fff80000.00000001 result=7ff80000.00000001 errno=0
func=log op1=7ff00000.00000001 result=7ff80000.00000001 errno=0 status=i
func=log op1=fff00000.00000001 result=7ff80000.00000001 errno=0 status=i
func=log op1=7ff00000.00000000 result=7ff00000.00000000 errno=0
func=log op1=fff00000.00000000 result=7ff80000.00000001 errno=EDOM status=i
func=log op1=7fefffff.ffffffff result=40862e42.fefa39ef.354 errno=0
func=log op1=ffefffff.ffffffff result=7ff80000.00000001 errno=EDOM status=i
func=log op1=3ff00000.00000000 result=00000000.00000000 errno=0
func=log op1=bff00000.00000000 result=7ff80000.00000001 errno=EDOM status=i
func=log op1=00000000.00000000 result=fff00000.00000000 errno=ERANGE status=z
func=log op1=80000000.00000000 result=fff00000.00000000 errno=ERANGE status=z
func=log op1=00000000.00000001 result=c0874385.446d71c3.639 errno=0
func=log op1=80000000.00000001 result=7ff80000.00000001 errno=EDOM status=i
func=log op1=40000000.00000000 result=3fe62e42.fefa39ef.358 errno=0
func=log op1=3fe00000.00000000 result=bfe62e42.fefa39ef.358 errno=0
