file(GLOB TEST_SOURCE_CPP *.cpp)

add_executable(wsolve-test ${TEST_SOURCE_CPP})
target_link_libraries(wsolve-test wsolve-lib)