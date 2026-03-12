cmake_minimum_required(VERSION 3.16)

project(EZTracker)

set(CMAKE_CXX_STANDARD 17)

file(GLOB_RECURSE SOURCES
    src/*.cpp
)

add_executable(tracker ${SOURCES})
