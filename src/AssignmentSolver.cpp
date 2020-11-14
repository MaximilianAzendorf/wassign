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

#include "AssignmentSolver.h"

#ifndef USE_CBC
#define USE_CBC
#endif

#include <ortools/linear_solver/linear_solver.h>

#include <utility>

set<pair<int, int>> AssignmentSolver::get_blocked_constraint_edges(shared_ptr<Scheduling const> const& scheduling)
{
    set<pair<int, int>> blockedEdges;

    for(Constraint constraint : _staticData->constraints)
    {
        switch(constraint.type())
        {
            case ChooserIsInChoice:
            {
                int s = scheduling->set_of(constraint.right());
                int from = _staticData->baseFlow.nodes().at(MipFlowStaticData::node_chooser(constraint.left(), s));

                for(int w = 0; w < _inputData->choice_count(); w++)
                {
                    if(constraint.right() == w || scheduling->set_of(w) != s) continue;

                    int to = _staticData->baseFlow.nodes().at(MipFlowStaticData::node_choice(w));
                    blockedEdges.insert(std::make_pair(from, to));
                }
                break;
            }
            case ChooserIsNotInChoice:
            {
                for(int s = 0; s < _inputData->set_count(); s++)
                {
                    int from = _staticData->baseFlow.nodes().at(MipFlowStaticData::node_chooser(constraint.left(), s));
                    int to = _staticData->baseFlow.nodes().at(MipFlowStaticData::node_choice(constraint.right()));
                    blockedEdges.insert(std::make_pair(from, to));
                }
                break;
            }
            case ChoicesHaveSameChoosers:
            {
                // This is handled elsewhere.
                //
                break;
            }
            default:
            {
                throw std::logic_error("This kind of constraint is not compatible with the min cost flow solver.");
            }
        }
    }

    return blockedEdges;
}

shared_ptr<Assignment const> AssignmentSolver::solve_with_limit(shared_ptr<Scheduling const> const& scheduling,
                                                                int preferenceLimit,
                                                                op::MPSolver& solver)
{
    MipFlow<flowid, flowid> flow(_staticData->baseFlow);

    for(int p = 0; p < _inputData->chooser_count(); p++)
    {
        for(int s = 0; s < _inputData->set_count(); s++)
        {
            flow.add_supply(flow.nodes().at(MipFlowStaticData::node_chooser(p, s)), 1);
        }
    }

    for(int w = 0; w < _inputData->choice_count(); w++)
    {
        flow.add_supply(flow.nodes().at(MipFlowStaticData::node_choice(w)), -_inputData->choice(w).min());
    }

    for(int s = 0; s < _inputData->set_count(); s++)
    {
        // Count the number of choosers that will already be absorbed by the choice nodes.
        //
        int coveredChoosers = 0;
        for(int w = 0; w < _inputData->choice_count(); w++)
        {
            if(scheduling->set_of(w) != s) continue;
            coveredChoosers += _inputData->choice(w).min();
        }

        flow.add_supply(
                flow.nodes().at(MipFlowStaticData::node_set(s)),
                -(_inputData->chooser_count() - coveredChoosers));
    }

    vector<int> edgesCap;
    vector<long> edgesCost;
    map<pair<int, int>, int> edgesIdx;
    int nextEdgeIdx = 0;

    for(int p = 0; p < _inputData->chooser_count(); p++)
    {
        for(int s = 0; s < _inputData->set_count(); s++)
        {
            for(int w = 0; w < _inputData->choice_count(); w++)
            {
                if(scheduling->set_of(w) != s || _inputData->chooser(p).preference(w) > preferenceLimit)
                    continue;

                edgesCap.push_back(1);
                edgesCost.push_back(
                        (long)pow(_inputData->chooser(p).preference(w) + 1.0, _options->preference_exponent()));

                edgesIdx[std::make_pair(
                        flow.nodes().at(MipFlowStaticData::node_chooser(p, s)),
                        flow.nodes().at(MipFlowStaticData::node_choice(w)))]
                        = nextEdgeIdx++;
            }
        }
    }

    for(int w = 0; w < _inputData->choice_count(); w++)
    {
        for(int s = 0; s < _inputData->set_count(); s++)
        {
            if(scheduling->set_of(w) != s) continue;

            edgesCap.push_back(_inputData->choice(w).max() - _inputData->choice(w).min());
            edgesCost.push_back(0);

            edgesIdx[std::make_pair(
                    flow.nodes().at(MipFlowStaticData::node_choice(w)),
                    flow.nodes().at(MipFlowStaticData::node_set(s)))]
                    = nextEdgeIdx++;
        }
    }

    // Remove all blocked edges
    //
    auto blockedEdges = get_blocked_constraint_edges(scheduling);
    blockedEdges.insert(_staticData->blockedEdges.begin(), _staticData->blockedEdges.end());

    for(auto blocked : blockedEdges)
    {
        edgesIdx.erase(blocked);
    }

    for(auto edge : edgesIdx)
    {
        int start = edge.first.first;
        int end = edge.first.second;
        int cap = edgesCap[edge.second];
        long cost = edgesCost[edge.second];

        flow.add_edge(MipFlowStaticData::edge_id(start, end), start, end, cap, cost);
    }

    // Create edge groups
    //
    for(vector<int> const& group : Constraints::get_dependent_choices(
            _inputData->assignment_constraints(),
            _inputData->choice_count()))
    {
        if(group.size() == 1) continue;

        for(int p = 0; p < _inputData->chooser_count(); p++)
        {
            vector<long> edgeGroup;
            for(int w : group)
            {
                int s = scheduling->set_of(w);
                int from = flow.nodes().at(MipFlowStaticData::node_chooser(p, s));
                int to = flow.nodes().at(MipFlowStaticData::node_choice(w));
                edgeGroup.push_back(MipFlowStaticData::edge_id(from, to));
            }

            flow.create_edge_group_or_block_edges(edgeGroup.begin(), edgeGroup.end());
        }
    }

    // ... and solve this instance
    //
    if(!flow.solve(solver))
    {
        return nullptr;
    }

    // Now we have to extract the assignment solution from the min cost flow solution
    //
    vector<vector<int>> data(_inputData->chooser_count(), vector<int>(_inputData->set_count(), -1));
    for(int p = 0; p < _inputData->chooser_count(); p++)
    {
        for(int s = 0; s < _inputData->set_count(); s++)
        {
            for(int w = 0; w < _inputData->choice_count(); w++)
            {
                int from = flow.nodes().at(MipFlowStaticData::node_chooser(p, s));
                int to = flow.nodes().at(MipFlowStaticData::node_choice(w));

                if(flow.solution_value_at(MipFlowStaticData::edge_id(from, to)) == 1)
                {
                    data[p][s] = w;
                }
            }
        }
    }

    return std::make_shared<Assignment const>(_inputData, data);
}

shared_ptr<Assignment const> AssignmentSolver::solve(const_ptr<Scheduling const> const& scheduling)
{
    auto solver = op::MPSolver("solver", op::MPSolver::CBC_MIXED_INTEGER_PROGRAMMING);

    int prefIdx = 0;
    for(; prefIdx < _inputData->preference_levels().size(); prefIdx++)
    {
        if(_inputData->preference_levels().at(prefIdx) == _csAnalysis->preference_bound()) break;
    }

    int minIdx = prefIdx;
    int maxIdx = _inputData->preference_levels().size();

    shared_ptr<Assignment const> bestAssignment = nullptr;

    // We do binary search through all possible preference limits to find the lowest one.
    //
    do
    {
        int prefLimit = _inputData->preference_levels().at(prefIdx);
        auto assignment = solve_with_limit(scheduling, prefLimit, solver);
        if(assignment == nullptr)
        {
            minIdx = prefIdx + 1;
        }
        else
        {
            bestAssignment = assignment;
            maxIdx = prefIdx - 1;
        }
        prefIdx = (maxIdx + minIdx) / 2;
    } while(maxIdx > minIdx);

    return bestAssignment;
}

AssignmentSolver::AssignmentSolver(const_ptr<InputData> inputData,
                                   const_ptr<CriticalSetAnalysis> csAnalysis,
                                   const_ptr<MipFlowStaticData> staticData,
                                   const_ptr<Options> options,
                                   cancel_token cancellation)
    : _inputData(std::move(inputData)),
    _csAnalysis(std::move(csAnalysis)),
    _staticData(std::move(staticData)),
    _options(std::move(options)),
    _cancellation(std::move(cancellation))
{
}
