cmake_minimum_required(VERSION 3.10)
project(Z80_Disassembler)
include_directories(include)

set(CMAKE_CXX_STANDARD 17)

set(CORE_CXX_FLAGS "-std=c++17 -m64 -pthread -Wstrict-aliasing -msse -msse2 -Wall")

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${CORE_CXX_FLAGS} -g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${CORE_CXX_FLAGS} -g -O2")


add_executable(Z80_Disassembler main.cpp src/Emulator.cpp include/Emulator.h)