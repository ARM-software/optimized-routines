# CMakeLists.txt
#
# Copyright (C) 2015, ARM Limited, All Rights Reserved
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

# This project will build the math library, and test executables. It imports
# a test case generator "rtest" which is compiled as a host application, in
# order to support cross-compilation and running the tests on the host using
# an emulator.
cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
project(armClib C)

# User didn't select a build type. Default to release.
if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif ()

# Suppress toolchain file warning.
if (CMAKE_TOOLCHAIN_FILE)
endif()

# Uncomment if you want more verbose make output.
# set(CMAKE_VERBOSE_MAKEFILE ON)

set(armClib_VERSION_MAJOR 1)
set(armClib_VERSION_MINOR 0)
set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Parameters to choose between different architectures and library variants.
# At the moment, we only support AArch64 and Cortex-A57/A53.
set(MATHLIB_TARGET_CPU "A57" CACHE STRING "Tune libraries for target cpu")
set(MATHLIB_TARGET_ARCH "AArch64" CACHE STRING "Set target architecture for code generation")

# Set basic cflags. TODO: Replace type punning with safe alternative and re-add
# strict aliasing warning.
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -std=c99 -Wno-strict-aliasing")

# Set architecture-specific emulation settings for the test suite.
# This assumes that the user has installed qemu-static on the host machine.
if (CMAKE_CROSSCOMPILING AND NOT CMAKE_CROSSCOMPILING_EMULATOR)
  if (MATHLIB_TARGET_ARCH MATCHES "AArch64") # AArch64 (ARMv8)
    set(CMAKE_CROSSCOMPILING_EMULATOR "qemu-aarch64-static")
  elseif (MATHLIB_TARGET_ARCH MATCHES "AArch32") # AArch32 (ARMv8)
    set(CMAKE_CROSSCOMPILING_EMULATOR "qemu-arm-static")
  endif ()
endif ()

# Add subdirectories for the math library and test suite.
include_directories("${PROJECT_BINARY_DIR}"
  "${PROJECT_SOURCE_DIR}/test"
  "${PROJECT_SOURCE_DIR}/math/include"
  "${PROJECT_SOURCE_DIR}/math")

add_subdirectory(test)
add_subdirectory(math)

set(EXTRA_LIBS ${EXTRA_LIBS} math)

include(ExternalProject)
ExternalProject_Add(rtest
  PREFIX "${CMAKE_BINARY_DIR}/rtest"
  SOURCE_DIR "${PROJECT_SOURCE_DIR}/test/rtest"
  INSTALL_DIR "${CMAKE_BINARY_DIR}/bin"
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR})

add_dependencies(mathtest rtest)
