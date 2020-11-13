#include "HillClimbingSolver.h"
#include "Util.h"

#include <utility>

int HillClimbingSolver::max_neighbor_key()
{
    return _inputData->workshop_count() * (_inputData->slot_count() - 1);
}

shared_ptr<Scheduling const> HillClimbingSolver::neighbor(shared_ptr<Scheduling const> const& scheduling, int neighborKey)
{
    vector<int> data(scheduling->raw_data());

    int s = neighborKey / _inputData->workshop_count();
    int w = neighborKey % _inputData->workshop_count();

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

HillClimbingSolver::HillClimbingSolver(const_ptr<InputData> inputData,
                                       const_ptr<CriticalSetAnalysis> csAnalysis,
                                       const_ptr<MipFlowStaticData> staticData,
                                       const_ptr<Scoring> scoring,
                                       const_ptr<Options> options,
                                       cancel_token cancellation)
    : _inputData(std::move(inputData)),
    _csAnalysis(std::move(csAnalysis)),
    _staticData(std::move(staticData)),
    _scoring(std::move(scoring)),
    _options(std::move(options)),
    _cancellation(std::move(cancellation)),
    _assignmentSolver(_inputData, _csAnalysis, _staticData, _options, _cancellation)
{
}

Solution HillClimbingSolver::solve(const_ptr<Scheduling> const& scheduling)
{
    Solution bestSolution(scheduling, _assignmentSolver.solve(scheduling));
    Score bestScore = _scoring->evaluate(bestSolution);

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

            if(is_set(_cancellation)) return Solution::invalid();

            Score neighborScore = _scoring->evaluate(neighborSolution);

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

