cmake_minimum_required(VERSION 3.15)

set(CMAKE_CXX_STANDARD 17)
project(wassign)

include_directories(build-wasm/or-tools/emscripten/upstream/emscripten/system/include)
include_directories(
        deps/chaiscript/include
        deps/catch2/single_include
        deps/popl/include
        deps/magic_enum/include)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wno-sign-compare")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

add_subdirectory(src)
add_subdirectory(test)
