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
#include "Solution.h"
#include "Scoring.h"
#include "CriticalSetAnalysis.h"
#include "MipFlowStaticData.h"
#include "Score.h"
#include "HillClimbingSolver.h"
#include "SchedulingSolver.h"

#include <shared_mutex>
#include <future>

struct ShotgunSolverProgress
{
    float schedDepth = 0;
    int iterations = 0;
    int assignments = 0;
    int lp = 0;
    Solution best_solution = Solution::invalid();
    Score best_score = {.major = INFINITY, .minor = INFINITY};
};

/**
 * Performs shotgun hill climbing on the input data (combining the HillClimbingSolver and SchedulingSolver classes).
 * Every iteration, a scheduling gets calculated and then hill climbing is performed on it.
 */
class ShotgunSolver
{
private:
    const_ptr<InputData> _inputData;
    const_ptr<Options> _options;
    cancel_token _cancellation;

    const_ptr<Scoring> _scoring;

    unique_ptr<HillClimbingSolver> _hillClimbingSolver;

    unique_ptr<SchedulingSolver> _schedulingSolver;

    ShotgunSolverProgress _progress;

    int _schedulings = 0;

public:
    ShotgunSolver(const_ptr<InputData> inputData,
                  const_ptr<CriticalSetAnalysis> const& csAnalysis,
                  const_ptr<MipFlowStaticData> const& staticData,
                  const_ptr<Scoring> scoring,
                  const_ptr<Options> options,
                  cancel_token cancellation = cancel_token());

    [[nodiscard]] Solution current_solution() const;

    [[nodiscard]] ShotgunSolverProgress const& progress();

    int iterate(int numberOfIterations = 1);
};


