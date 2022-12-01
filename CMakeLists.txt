cmake_minimum_required(VERSION 3.16)
project(OS1)

set(CMAKE_CXX_STANDARD 14)

add_executable(OS1 smash.cpp Commands.cpp signals.cpp)
