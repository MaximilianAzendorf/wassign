file(GLOB TEST_SOURCE_CPP *.cpp)

add_executable(wassign-test ${TEST_SOURCE_CPP})
target_link_libraries(wassign-test wassign-lib)
