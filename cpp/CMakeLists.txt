cmake_minimum_required(VERSION 3.15)
project(wsolve)

include_directories("/usr/local/include")

set(CMAKE_CXX_STANDARD 17)

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
        Scoring.h MipFlow.h MipSolver.h MipFlowStaticData.h)
