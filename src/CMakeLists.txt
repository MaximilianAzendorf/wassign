include_directories("/usr/local/include")

set(GFLAGS_USE_TARGET_NAMESPACE ON)
set(Boost_USE_STATIC_LIBS ON)

find_package(Boost COMPONENTS program_options thread REQUIRED)
find_package(TBB REQUIRED)
find_package(ortools CONFIG REQUIRED)

file(GLOB SOURCE_CPP *.cpp)
file(GLOB SOURCE_IPP *.ipp)
file(GLOB SOURCE_H *.h)

list(FILTER SOURCE_CPP EXCLUDE REGEX ".*main.cpp$")

add_library(wsolve-lib STATIC ${SOURCE_CPP} ${SOURCE_H} ${SOURCE_IPP})
target_link_libraries(wsolve-lib ${Boost_LIBRARIES} tbb ortools::ortools)

add_executable(wsolve main.cpp)
target_link_libraries(wsolve wsolve-lib)
