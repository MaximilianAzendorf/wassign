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

#include "HillClimbingSolver.h"
#include "Util.h"

#include <utility>

int HillClimbingSolver::max_neighbor_key()
{
    return _inputData->choice_count() * (_inputData->set_count() - 1);
}

shared_ptr<Assignment const> HillClimbingSolver::solve_assignment(const_ptr<Scheduling const> const& scheduling)
{
    auto res = _assignmentSolver.solve(scheduling);
    _assignmentCount++;
    return res;
}

shared_ptr<Scheduling const> HillClimbingSolver::neighbor(shared_ptr<Scheduling const> const& scheduling, int neighborKey)
{
    vector<int> data(scheduling->raw_data());

    int s = neighborKey / _inputData->choice_count();
    int w = neighborKey % _inputData->choice_count();

    if(s >= scheduling->set_of(w))
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

int HillClimbingSolver::assignment_count() const
{
    return _assignmentCount;
}

Solution HillClimbingSolver::solve(const_ptr<Scheduling> const& scheduling)
{
    Solution bestSolution(scheduling, solve_assignment(scheduling));
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
            Solution neighborSolution(neighbor, solve_assignment(neighbor));

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

int HillClimbingSolver::lp_count() const
{
    return _assignmentSolver.lp_count();
}

