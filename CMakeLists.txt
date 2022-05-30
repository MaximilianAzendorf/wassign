cmake_minimum_required(VERSION 3.15)

set(CMAKE_CXX_STANDARD 17)
project(wassign)

include_directories(
        deps/chaiscript/include
        deps/chaiscript_extras/include
        deps/catch2/single_include
        deps/popl/include
        deps/magic_enum/include
        deps/rapidcsv/src)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(GFLAGS_USE_TARGET_NAMESPACE ON)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    find_package(TBB REQUIRED)
endif()

set(BUILD_DEPS ON)
set(BUILD_EXAMPLES OFF)
set(BUILD_SAMPLES OFF)
set(USE_SCIP OFF)
set(BUILD_SHARED_LIBS OFF)
add_subdirectory(deps/or-tools)

set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wno-sign-compare")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O2")

add_subdirectory(src)
add_subdirectory(test)
