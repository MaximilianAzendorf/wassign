#pragma once

#include "Types.h"
#include "Solution.h"
#include "Scoring.h"
#include "CriticalSetAnalysis.h"
#include "MipFlowStaticData.h"
#include "Score.h"
#include "MipFlowStaticData.h"

#include <future>
#include <climits>
#include <utility>

namespace op = operations_research;

/**
 * Class for calculating an optimal assignment for a given scheduling.
 */
class AssignmentSolver
{
private:
    const_ptr<InputData> _inputData;
    const_ptr<CriticalSetAnalysis> _csAnalysis;
    const_ptr<MipFlowStaticData> _staticData;
    const_ptr<Options> _options;
    cancel_token _cancellation;

    /**
     * Calculates edges in the flow graph that have to be removed from the flow graph.
     */
    set<pair<int, int>> get_blocked_constraint_edges(shared_ptr<Scheduling const> const& scheduling);

    /**
     * Calculates an optimal assignment for the given scheduling, considering the given preference limit.
     */
     shared_ptr<Assignment const> solve_with_limit(shared_ptr<Scheduling const> const& scheduling,
                                                   int preferenceLimit,
                                                   op::MPSolver& solver);
public:
    /**
     * Constructor.
     *
     * @param staticData The static flow graph data for the given input data.
     */
    AssignmentSolver(const_ptr<InputData> inputData,
                     const_ptr<CriticalSetAnalysis> csAnalysis,
                     const_ptr<MipFlowStaticData> staticData,
                     const_ptr<Options> options,
                     cancel_token cancellation = cancel_token());

    /**
     * Calculates an optimal assignment for the given scheduling.
     */
    shared_ptr<Assignment const> solve(shared_ptr<Scheduling const> const& scheduling);
};


