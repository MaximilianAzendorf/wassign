include_directories("/usr/local/include")

set(GFLAGS_USE_TARGET_NAMESPACE ON)

find_package(TBB REQUIRED)
find_package(ortools CONFIG REQUIRED)

file(GLOB_RECURSE SOURCE_CPP *.cpp)
file(GLOB_RECURSE SOURCE_IPP *.ipp)
file(GLOB_RECURSE SOURCE_H *.h)

list(FILTER SOURCE_CPP EXCLUDE REGEX ".*main.cpp$")

add_library(wassign-lib STATIC ${SOURCE_CPP} ${SOURCE_H} ${SOURCE_IPP})
target_link_libraries(wassign-lib tbb ortools::ortools)

add_executable(wassign main.cpp)
target_link_libraries(wassign wassign-lib)
