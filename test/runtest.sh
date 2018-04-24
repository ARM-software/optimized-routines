#!/bin/bash

# runtest.sh - script to execute all mathlib tests
#
# Copyright (c) 2015, Arm Limited.
# SPDX-License-Identifier: Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# cd to bin directory.
cd "${0%/*}"

# Run the mathtest executable using input generated with rtest.
# For a native build, all executables can just be run and should work fine.
# For a cross-build, the actual tests have to be executed in QEMU or an emulator
# of choice. This is set in the CMake script and filled in here.

find . -iname "*.tst" -print0 | xargs -0 cat | ./rtest | "$@"
