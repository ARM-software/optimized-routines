# CMakeLists.txt
#
# Copyright (C) 2015-2017, ARM Limited, All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may
# not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# This file is part of the Optimized Routines project

# Use -frounding-math so gcc doesn't constant fold away expressions that raise
# exceptions in funder.c and dunder.c.
set_source_files_properties(funder.c dunder.c math_errf.c PROPERTIES COMPILE_FLAGS -frounding-math)

# Add files to the library.
set(mathlib_src
  e_expf.c e_exp2f.c e_logf.c e_log2f.c e_powf.c
  e_exp2f_data.c e_logf_data.c e_log2f_data.c e_powf_log2_data.c
  math_errf.c
  e_rem_pio2.c
  dunder.c funder.c ieee_status.c poly.c rredf.c
  k_cos.c s_cos.c
  s_cosf.c
  k_sin.c s_sin.c
  s_sinf.c
  k_tan.c s_tan.c
  s_tanf.c)

add_library(mathlib SHARED ${mathlib_src})
# Build a static library for cross-testing purposes (QEMU).
add_library(mathlib_static STATIC ${mathlib_src})

# Set CPU-specific tuning and compatibility flags.
if (MATHBENCH_TARGET_CPU MATCHES "A57")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mcpu=cortex-a57")
elseif (MATHBENCH_TARGET_CPU MATCHES "A53")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mcpu=cortex-a53")
else ()
  # Tuning for other CPUs is not supported yet.
endif ()

# Link against libc.
target_link_libraries(mathlib c)
target_link_libraries(mathlib_static c)

# Install the shared library to the build directory.
install(TARGETS mathlib DESTINATION ${CMAKE_BINARY_DIR}/lib)
install(TARGETS mathlib_static DESTINATION ${CMAKE_BINARY_DIR}/lib)
configure_file(
  ${PROJECT_SOURCE_DIR}/math/include/arm_math.h
  ${PROJECT_BINARY_DIR}/include/arm_math.h
  COPYONLY
  )
