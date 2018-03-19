; tan.tst - Directed test cases for tangent
;
; Copyright (C) 1999-2015, ARM Limited, All Rights Reserved
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

func=tan op1=7ff80000.00000001 result=7ff80000.00000001 errno=0
func=tan op1=fff80000.00000001 result=7ff80000.00000001 errno=0
func=tan op1=7ff00000.00000000 result=7ff80000.00000001 errno=EDOM status=i
func=tan op1=fff00000.00000000 result=7ff80000.00000001 errno=EDOM status=i
func=tan op1=00000000.00000000 result=00000000.00000000 errno=0
func=tan op1=80000000.00000000 result=80000000.00000000 errno=0
func=tan op1=00000000.00000001 result=00000000.00000001 errno=0 status=ux
func=tan op1=80000000.00000001 result=80000000.00000001 errno=0 status=ux
