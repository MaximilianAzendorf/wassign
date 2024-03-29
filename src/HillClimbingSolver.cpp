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
    return _inputData->choice_count() * (_inputData->slot_count());
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

    int s = neighborKey / _inputData->choice_count() - 1;
    int w = neighborKey % _inputData->choice_count();

    if(s >= scheduling->slot_of(w))
    {
        s += 1;
    }

    data[w] = s;

    auto newScheduling = std::make_shared<Scheduling const>(_inputData, data);
    return newScheduling;
}

shared_ptr<Scheduling const> HillClimbingSolver::random_swap_neighbor(shared_ptr<Scheduling const> const& scheduling)
{
    vector<int> data(scheduling->raw_data());
    vector<int> swapIdx;
    swapIdx.push_back(Rng::next(0, (int)data.size()));

    do
    {
        int nextIdx;
        do
        {
            nextIdx = Rng::next(0, (int)data.size());
        } while(std::find(swapIdx.begin(), swapIdx.end(), nextIdx) != swapIdx.end());
        swapIdx.push_back(nextIdx);

    } while(Rng::next(0, 3) == 0 && swapIdx.size() < data.size() / 2);

    int carry = data[swapIdx[swapIdx.size() - 1]];
    for(int idx : swapIdx)
    {
        std::swap(carry, data[idx]);
    }

    auto newScheduling = std::make_shared<Scheduling const>(_inputData, data);
    return newScheduling;
}

vector<shared_ptr<Scheduling const>> HillClimbingSolver::pick_neighbors(shared_ptr<Scheduling const> const& scheduling)
{
    bool addSwapNeighbors = scheduling->input_data().choice_count() > 1 && scheduling->input_data().slot_count() > 1;

    vector<shared_ptr<Scheduling const>> result;

    vector<int> neighborKeys(max_neighbor_key());
    std::iota(neighborKeys.begin(), neighborKeys.end(), 0);

    if(max_neighbor_key() > _options->max_neighbors())
    {
        std::shuffle(neighborKeys.begin(), neighborKeys.end(), Rng::engine());
    }

    for(int keyIdx = 0; keyIdx < neighborKeys.size(); keyIdx++)
    {
        if(result.size() >= _options->max_neighbors())
        {
            break;
        }

        if(keyIdx > _options->max_neighbors() * 32) break;

        int neighborKey = neighborKeys[keyIdx];
        auto nextNeighbor = neighbor(scheduling, neighborKey);
        if(!nextNeighbor->is_feasible()) continue;

        result.push_back(nextNeighbor);

        if (addSwapNeighbors)
        {
            auto swapNeighbor = random_swap_neighbor(scheduling);
            if (swapNeighbor->is_feasible())
            {
                result.push_back(swapNeighbor);
            }
        }
    }

    if (addSwapNeighbors && result.size() < _options->max_neighbors())
    {
        // Fill up the neighbors with swap neighbors
        int amount = std::min(_options->max_neighbors() - (int)result.size(), max_neighbor_key());
        for(int i = 0; i < amount * 32 && result.size() < _options->max_neighbors(); i++)
        {
            auto swapNeighbor = random_swap_neighbor(scheduling);
            if (swapNeighbor->is_feasible())
            {
                result.push_back(swapNeighbor);
            }
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
    if(bestSolution.is_invalid())
    {
        return Solution::invalid();
    }

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

