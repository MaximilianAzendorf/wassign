#include "HillClimbingSolver.h"
#include "Util.h"

#include <utility>

int HillClimbingSolver::max_neighbor_key()
{
    return _inputData.workshop_count() * (_inputData.slot_count() - 1);
}

shared_ptr<Scheduling const> HillClimbingSolver::neighbor(shared_ptr<Scheduling const> const& scheduling, int neighborKey)
{
    vector<int> data(scheduling->raw_data());

    int s = neighborKey / _inputData.workshop_count();
    int w = neighborKey % _inputData.workshop_count();

    if(s >= scheduling->slot_of(w))
    {
        s += 1;
    }

    int origs = data[w];
    data[w] = s;
    auto newScheduling = std::make_shared<Scheduling const>(_inputData, data);
    data[w] = origs;

    return newScheduling;
}

vector<shared_ptr<Scheduling const>> HillClimbingSolver::pick_neighbors(shared_ptr<Scheduling const> const& scheduling)
{
    vector<shared_ptr<Scheduling const>> result;

    vector<int> neighborKeys(max_neighbor_key());
    std::iota(neighborKeys.begin(), neighborKeys.end(), 0);

    if(max_neighbor_key() > MaxNeighborsPerIteration)
    {
        std::shuffle(neighborKeys.begin(), neighborKeys.end(), Rng::engine());
    }

    for(int neighborKey : neighborKeys)
    {
        auto nextNeighbor = neighbor(scheduling, neighborKey);
        if(!nextNeighbor->is_feasible()) continue;

        result.push_back(nextNeighbor);

        if(result.size() >= MaxNeighborsPerIteration)
        {
            break;
        }
    }

    return result;
}

HillClimbingSolver::HillClimbingSolver(InputData const& inputData,
                                       const CriticalSetAnalysis& csAnalysis,
                                       MipFlowStaticData const& staticData,
                                       Scoring const& scoring,
                                       Options const& options,
                                       std::shared_future<void> cancellation)
    : _inputData(inputData),
    _csAnalysis(csAnalysis),
    _staticData(staticData),
    _scoring(scoring),
    _options(options),
    _cancellation(std::move(cancellation)),
    _assignmentSolver(inputData, csAnalysis, staticData, options, cancellation)
{
}

Solution HillClimbingSolver::solve(shared_ptr<Scheduling const> const& scheduling)
{
    if(is_set(_cancellation)) return Solution::invalid();

    Solution bestSolution(scheduling, _assignmentSolver.solve(scheduling));
    Score bestScore = _scoring.evaluate(bestSolution);

    if(!bestScore.is_finite())
    {
        return Solution::invalid();
    }

    while(true)
    {
        bool foundBetterNeighbor = false;
        for(auto const& neighbor : pick_neighbors(bestSolution.scheduling()))
        {
            Solution neighborSolution(neighbor, _assignmentSolver.solve(neighbor));
            Score neighborScore = _scoring.evaluate(neighborSolution);

            if(neighborScore < bestScore)
            {
                foundBetterNeighbor = true;
                bestScore = neighborScore;
                bestSolution = neighborSolution;
            }
        }

        if(!foundBetterNeighbor)
        {
            break;
        }
    }

    return bestSolution;
}

