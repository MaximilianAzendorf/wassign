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
    InputData const& _inputData;
    CriticalSetAnalysis const& _csAnalysis;
    MipFlowStaticData const& _staticData;
    Options const& _options;
    std::shared_future<void> _cancellation;

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
    AssignmentSolver(InputData const& inputData,
                     CriticalSetAnalysis const& csAnalysis,
                     MipFlowStaticData const& staticData,
                     Options const& options,
                     std::shared_future<void> cancellation = std::shared_future<void>());

    /**
     * Calculates an optimal assignment for the given scheduling.
     */
    shared_ptr<Assignment const> solve(shared_ptr<Scheduling const> const& scheduling);
};


