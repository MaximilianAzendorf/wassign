cmake_minimum_required(VERSION 3.15)

set(CMAKE_CXX_STANDARD 17)
project(wsolve)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wno-sign-compare")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O2")

add_subdirectory(src)
add_subdirectory(test)