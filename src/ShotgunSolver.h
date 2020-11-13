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
    int iterations = 0;
    Solution best_solution = Solution::invalid();
    Score best_score = {.major = INFINITY, .minor = INFINITY};
};

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

public:
    ShotgunSolver(const_ptr<InputData> inputData,
                  const_ptr<CriticalSetAnalysis> const& csAnalysis,
                  const_ptr<MipFlowStaticData> const& staticData,
                  const_ptr<Scoring> scoring,
                  const_ptr<Options> options,
                  cancel_token cancellation = cancel_token());

    [[nodiscard]] Solution current_solution() const;

    [[nodiscard]] ShotgunSolverProgress const& progress() const;

    int iterate(int numberOfIterations = 1);
};


