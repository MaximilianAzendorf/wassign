#include "ShotgunSolver.h"

#include <utility>

#include "Status.h"
#include "SchedulingSolver.h"
#include "Score.h"

Solution ShotgunSolver::current_solution() const
{
    return _progress.best_solution;
}

ShotgunSolver::ShotgunSolver(const_ptr<InputData> inputData,
                             const_ptr<CriticalSetAnalysis> const& csAnalysis,
                             const_ptr<MipFlowStaticData> const& staticData,
                             const_ptr<Scoring> scoring,
                             const_ptr<Options> options,
                             cancel_token cancellation)
    : _inputData(std::move(inputData)),
    _options(std::move(options)),
    _cancellation(std::move(cancellation)),
    _scoring(std::move(scoring))
{
    _hillClimbingSolver = std::make_unique<HillClimbingSolver>(_inputData, csAnalysis, staticData, _scoring, _options);
    _schedulingSolver = std::make_unique<SchedulingSolver>(_inputData, csAnalysis, _options);

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

        if(is_set(_cancellation)) break;

        Score score = _scoring->evaluate(solution);

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
