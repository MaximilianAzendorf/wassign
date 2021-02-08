/*
 * Copyright 2020 Maximilian Azendorf
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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

/**
 * Performs hill climbing, starting with a given scheduling. The search space for the hill climbing is the space of all
 * valid schedulings (so the scheduling will be mutated over and over again until no better solution can be found this
 * way).
 */
class HillClimbingSolver
{
private:
    const_ptr<InputData> _inputData;
    const_ptr<CriticalSetAnalysis> _csAnalysis;
    const_ptr<MipFlowStaticData> _staticData;
    const_ptr<Scoring> _scoring;
    const_ptr<Options> _options;
    cancel_token _cancellation;

    int _assignmentCount = 0;

    AssignmentSolver _assignmentSolver;

    /**
     * The maximum number of possible mutations is also the maximum neighbor key.
     */
    int max_neighbor_key();

    /**
     * Solves the assignment for a given scheduling using the assignment solver.
     */
    shared_ptr<Assignment const> solve_assignment(const_ptr<Scheduling const> const& scheduling);

    /**
     * Returns a single neighbor (a scheduling differing by a single workshop-slot-assignment) of the given scheduling.
     * Which neighbor is determined by the neighbor key (between 0 and max_neighbor_key). Note that neighbors do not
     * have to necessarily be valid schedulings.
     */
    shared_ptr<Scheduling const> neighbor(shared_ptr<Scheduling const> const& scheduling, int neighborKey);

    /**
     * Returns a list of valid neighbors for the given scheduling.
     */
    vector<shared_ptr<Scheduling const>> pick_neighbors(shared_ptr<Scheduling const> const& scheduling);

public:
    /**
     * Constructor.
     */
    HillClimbingSolver(const_ptr<InputData> inputData,
                       const_ptr<CriticalSetAnalysis> csAnalysis,
                       const_ptr<MipFlowStaticData> staticData,
                       const_ptr<Scoring> scoring,
                       const_ptr<Options> options,
                       cancel_token cancellation = cancel_token());

    /**
     * Returns the number of times the assignment solver was invoked by this instance so far.
     */
    [[nodiscard]] int assignment_count() const;

    /**
     * Returns the number of linear programming instances solved by the assignment solver of this instance so far.
     */
    [[nodiscard]] int lp_count() const;

    /**
     * Performs hill climbing on the given scheduling and returns the resulting solution.
     */
    Solution solve(const_ptr<Scheduling> const& scheduling);
};


