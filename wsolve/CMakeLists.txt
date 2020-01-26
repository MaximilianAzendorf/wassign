cmake_minimum_required(VERSION 3.15)
project(wsolve)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wno-sign-compare")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O2")

include_directories("/usr/local/include")

set(CMAKE_CXX_STANDARD 17)

set(GFLAGS_USE_TARGET_NAMESPACE ON)
set(Boost_USE_STATIC_LIBS ON)

find_package(Boost COMPONENTS program_options thread REQUIRED)
find_package(TBB REQUIRED)
find_package(ortools CONFIG REQUIRED)

add_executable(wsolve main.cpp
        UnionFind.h
        Types.h
        CriticalSet.h
        InputData.h
        WorkshopData.h
        ParticipantData.h
        SlotData.h
        Constraint.h
        MutableInputData.h
        Scheduling.h
        InputReader.h
        Status.h
        Options.h
        Util.h
        CriticalSetAnalysis.h
        Assignment.h
        Solution.h
        SchedulingSolver.h
        Rng.h
        Scoring.h
        MipFlow.h
        MipSolver.h
        MipFlowStaticData.h
        Constraints.h
        InputException.h
        ConstraintParser.h
        Version.h
        OutputWriter.h SpiritUtil.h FuzzyMatch.h)

target_link_libraries(wsolve ${Boost_LIBRARIES} tbb ortools::ortools)
