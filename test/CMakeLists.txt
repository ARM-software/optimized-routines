# CMakeLists.txt
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

# Turn off spurious warning about missing braces around initialiser.
set_source_files_properties(mathtest.c PROPERTIES COMPILE_FLAGS -Wno-missing-braces)

# Only build the test suite if make check is invoked.
add_executable(mathtest EXCLUDE_FROM_ALL mathtest.c)

# Compile a mathtest binary that tests the -lm math library.
add_executable(mathtest_local EXCLUDE_FROM_ALL mathtest.c)

if (CMAKE_CROSSCOMPILING)
  # Statically link the mathtest executable so it works with qemu-static.
  set_target_properties(mathtest PROPERTIES LINK_FLAGS -static)
  target_link_libraries(mathtest mathlib_static m)
  # For testing local (GNU) libm.
  set_target_properties(mathtest_local PROPERTIES LINK_FLAGS -static COMPILE_FLAGS -DTEST_LOCAL_LIBM)
  target_link_libraries(mathtest_local mathlib_static m)
else ()
  target_link_libraries(mathtest mathlib m)
  # Set -D flag to use local libm as above.
  set_target_properties(mathtest_local PROPERTIES COMPILE_FLAGS -DTEST_LOCAL_LIBM)
  target_link_libraries(mathtest_local mathlib m)
endif ()

configure_file(
  ${PROJECT_SOURCE_DIR}/test/runtest.sh
  ${PROJECT_BINARY_DIR}/bin/runtest.sh
  @ONLY)

add_custom_command(
  TARGET mathtest POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
  ${CMAKE_SOURCE_DIR}/test/testcases $<TARGET_FILE_DIR:mathtest>
  )

add_custom_target(check COMMAND ${PROJECT_BINARY_DIR}/bin/runtest.sh ${PROJECT_BINARY_DIR}/bin/mathtest)
add_dependencies(check mathtest)

add_custom_target(check-local COMMAND ${PROJECT_BINARY_DIR}/bin/runtest.sh ${PROJECT_BINARY_DIR}/bin/mathtest_local)
add_dependencies(check-local mathtest mathtest_local)
