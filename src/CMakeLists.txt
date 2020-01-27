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
        UnionFind.ipp
        Types.h
        CriticalSet.h
        CriticalSet.cpp
        InputData.h
        InputData.cpp
        WorkshopData.h
        WorkshopData.cpp
        ParticipantData.h
        ParticipantData.cpp
        SlotData.h
        SlotData.cpp
        Constraint.h
        Constraint.cpp
        MutableInputData.h
        Scheduling.h
        Scheduling.cpp
        InputReader.h
        InputReader.cpp
        Status.h
        Status.cpp
        Options.h
        Options.cpp
        Util.h
        CriticalSetAnalysis.h
        CriticalSetAnalysis.cpp
        Assignment.h
        Assignment.cpp
        Solution.h
        Solution.cpp
        SchedulingSolver.h
        SchedulingSolver.cpp
        Rng.h
        Rng.cpp
        Score.h
        Score.cpp
        Scoring.h
        Scoring.cpp
        MipFlow.h
        MipFlow.ipp
        MipSolver.h
        MipSolver.cpp
        MipFlowStaticData.h
        Constraints.h
        Constraints.cpp
        InputException.h
        InputException.cpp
        ConstraintParser.h
        ConstraintParser.cpp
        ConstraintParser.ipp
        Version.h
        OutputWriter.h
        OutputWriter.cpp
        SpiritUtil.h
        FuzzyMatch.h
        FuzzyMatch.cpp
        InputData.ipp
        )

target_link_libraries(wsolve ${Boost_LIBRARIES} tbb ortools::ortools)
