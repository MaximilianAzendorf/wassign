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
    const_ptr<InputData> _inputData;
    const_ptr<CriticalSetAnalysis> _csAnalysis;
    const_ptr<MipFlowStaticData> _staticData;
    const_ptr<Scoring> _scoring;
    const_ptr<Options> _options;
    cancel_token _cancellation;

    AssignmentSolver _assignmentSolver;

    int max_neighbor_key();

    shared_ptr<Scheduling const> neighbor(shared_ptr<Scheduling const> const& scheduling, int neighborKey);

    vector<shared_ptr<Scheduling const>> pick_neighbors(shared_ptr<Scheduling const> const& scheduling);

public:
    inline static const int MaxNeighborsPerIteration = 16;

    /**
     * Constructor.
     */
    HillClimbingSolver(const_ptr<InputData> inputData,
                       const_ptr<CriticalSetAnalysis> csAnalysis,
                       const_ptr<MipFlowStaticData> staticData,
                       const_ptr<Scoring> scoring,
                       const_ptr<Options> options,
                       cancel_token cancellation = cancel_token());

    Solution solve(const_ptr<Scheduling> const& scheduling);
};


