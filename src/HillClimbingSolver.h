#pragma once

#include "Types.h"
#include "Options.h"
#include "InputData.h"
#include "MipFlowStaticData.h"
#include "Scheduling.h"
#include "CriticalSetAnalysis.h"
#include "Scoring.h"
#include "AssignmentSolver.h"

#include <future>

class HillClimbingSolver
{
private:
    InputData const& _inputData;
    CriticalSetAnalysis const& _csAnalysis;
    MipFlowStaticData const& _staticData;
    Scoring const& _scoring;
    Options const& _options;
    std::shared_future<void> _cancellation;

    AssignmentSolver _assignmentSolver;

    int max_neighbor_key();

    shared_ptr<Scheduling const> neighbor(shared_ptr<Scheduling const> const& scheduling, int neighborKey);

    vector<shared_ptr<Scheduling const>> pick_neighbors(shared_ptr<Scheduling const> const& scheduling);

public:
    inline static const int MaxNeighborsPerIteration = 16;

    /**
     * Constructor.
     */
    HillClimbingSolver(InputData const& inputData,
                       CriticalSetAnalysis const& csAnalysis,
                       MipFlowStaticData const& staticData,
                       Scoring const& scoring,
                       Options const& options,
                       std::shared_future<void> cancellation = std::shared_future<void>());

    Solution solve(shared_ptr<Scheduling const> const& scheduling);
};


