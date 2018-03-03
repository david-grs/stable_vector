cmake_minimum_required(VERSION 3.1)

project(stable_vector)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
	set(cxx_compile_options "-g -Wall -Wextra -Wpedantic -Wconversion -Wswitch-enum -Wunreachable-code -Wwrite-strings -Wcast-align -Wshadow -Wundef -Wold-style-cast -Wshadow -Wdouble-promotion")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${cxx_compile_options}")
endif()

include(gtest.cmake)

find_package(Threads) # needed by gtest
add_executable(tests unit_tests.cc)
target_link_libraries(tests gtest ${CMAKE_THREAD_LIBS_INIT})

