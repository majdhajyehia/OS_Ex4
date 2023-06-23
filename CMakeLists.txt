cmake_minimum_required(VERSION 3.16)
project(Ex4)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)

add_executable(Ex4
        MemoryConstants.h
        PhysicalMemory.cpp
        PhysicalMemory.h
        SimpleTest.cpp
        VirtualMemory.cpp
        VirtualMemory.h)
