cmake_minimum_required(VERSION 3.11)

set(CMAKE_CXX_STANDARD 11)

SET(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Debug type")

project(ebnf)

add_executable(sciconf
    ebnf.hpp
    ebnf.cpp
    ebnf_parser.cpp
    ebnf_parser.hpp
    ebnf_test.cpp
)




