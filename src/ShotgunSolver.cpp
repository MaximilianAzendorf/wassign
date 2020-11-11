#include "ShotgunSolver.h"

#include "Status.h"
#include "SchedulingSolver.h"
#include "Constraints.h"
#include "Score.h"

#include <climits>
#include <utility>
/*
CriticalSetAnalysis const& ShotgunSolver::get_cs_analysis(InputData const& inputData)
{
    if(_csAnalysis == nullptr)
    {
        if (!_options.no_critical_sets() && inputData.slot_count() > 1)
        {
            Status::info("Performing critical set analysis.");
            _csAnalysis = std::make_unique<CriticalSetAnalysis>(inputData);
            Status::info(str(_csAnalysis->sets().size()) + " critical set(s) found; preference limit is " +
                         str(_csAnalysis->preference_bound()) + ".");
        }
        else
        {
            if (inputData.slot_count() > 1)
            {
                Status::info("Skipping critical set analysis.");
            }
            else
            {
                Status::info("No critical set analysis needed for single slot.");
            }

            _csAnalysis = std::make_unique<CriticalSetAnalysis>(inputData, false);
        }
    }

    return *_csAnalysis;
}*/

Solution ShotgunSolver::current_solution() const
{
    return _progress.best_solution;
}

ShotgunSolver::ShotgunSolver(InputData const& inputData,
                             CriticalSetAnalysis const& csAnalysis,
                             MipFlowStaticData const& staticData,
                             Scoring const& scoring,
                             Options const& options)
    : _inputData(inputData),
    _options(options),
    _scoring(scoring)
{
    _hillClimbingSolver = std::make_unique<HillClimbingSolver>(inputData, csAnalysis, staticData, scoring, options);
    _schedulingSolver = std::make_unique<SchedulingSolver>(inputData, csAnalysis, options);

    _progress.best_score = {.major = INFINITY, .minor = INFINITY};
    _progress.best_solution = Solution::invalid();
}

int ShotgunSolver::iterate(int numberOfIterations)
{
    int iteration = 0;
    for(; iteration < numberOfIterations; iteration++)
    {
        if(!_schedulingSolver->next_scheduling())
        {
            break;
        }

        Solution solution = _hillClimbingSolver->solve(_schedulingSolver->scheduling());
        Score score = _scoring.evaluate(solution);

        if(score < _progress.best_score)
        {
            _progress.best_solution = solution;
            _progress.best_score = score;
        }

        _progress.iterations++;
    }
    return iteration;
}

ShotgunSolverProgress const& ShotgunSolver::progress() const
{
    return _progress;
}
