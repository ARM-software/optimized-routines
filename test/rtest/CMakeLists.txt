cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

add_executable(rtest main.c dotest.c random.c semi.c wrappers.c)

find_library(MPFR_LIB mpfr)
find_library(MPC_LIB mpc)

target_link_libraries(rtest m mpfr mpc)

install(TARGETS rtest DESTINATION bin)
