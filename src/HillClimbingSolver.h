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

class HillClimbingSolver
{
private:
    const_ptr<InputData> _inputData;
    const_ptr<CriticalSetAnalysis> _csAnalysis;
    const_ptr<MipFlowStaticData> _staticData;
    const_ptr<Scoring> _scoring;
    const_ptr<Options> _options;
    cancel_token _cancellation;

    int _assignmentCount;

    AssignmentSolver _assignmentSolver;

    int max_neighbor_key();

    shared_ptr<Assignment const> solve_assignment(const_ptr<Scheduling const> const& scheduling);

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

    [[nodiscard]] int assignment_count() const;
    [[nodiscard]] int lp_count() const;

    Solution solve(const_ptr<Scheduling> const& scheduling);
};


