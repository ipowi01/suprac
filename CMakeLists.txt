cmake_minimum_required(VERSION 3.31)
project(suprac)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lstdc++exp")

add_executable(suprac main.cpp)
