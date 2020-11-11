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
    int iterations;
    Solution best_solution;
    Score best_score;
};

class ShotgunSolver
{
private:
    InputData const& _inputData;
    Options const& _options;

    Scoring _scoring;

    unique_ptr<HillClimbingSolver> _hillClimbingSolver;

    unique_ptr<SchedulingSolver> _schedulingSolver;

    ShotgunSolverProgress _progress;

public:
    ShotgunSolver(InputData const& inputData,
                  CriticalSetAnalysis const& csAnalysis,
                  MipFlowStaticData const& staticData,
                  Scoring const& scoring,
                  Options const& options);

    Solution current_solution() const;

    ShotgunSolverProgress const& progress() const;

    int iterate(int numberOfIterations = 1);
};


