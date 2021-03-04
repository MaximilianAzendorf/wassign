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

#include "Constraints.h"

#include <utility>

void AssignmentSolver::create_implications(const_ptr<Scheduling> const& scheduling, MipFlow<flowid, flowid>& flow)
{
    for(Constraint constraint : _inputData->assignment_constraints())
    {
        if(constraint.type() != ChoosersHaveSameChoices) continue;

        switch(constraint.type())
        {
            case ChoosersOfChoicesRelation:
            {
                break;
            }
            case ChoicesOfChoosersRelation:
            {
                break;
            }
            default: continue;
        }

        for (int s = 0; s < _inputData->slot_count(); s++)
        {
            for (int w = 0; w < _inputData->choice_count(); w++)
            {
                int from1 = _staticData->baseFlow.nodes().at(MipFlowStaticData::node_chooser(constraint.left(), s));
                int to1 = _staticData->baseFlow.nodes().at(MipFlowStaticData::node_choice(w));
                int from2 = _staticData->baseFlow.nodes().at(MipFlowStaticData::node_chooser(constraint.right(), s));
                int to2 = _staticData->baseFlow.nodes().at(MipFlowStaticData::node_choice(w));

                vector<flowid> edgeGroup { MipFlowStaticData::edge_id(from1, to1), MipFlowStaticData::edge_id(from2, to2) };
                flow.make_edges_equal(edgeGroup.begin(), edgeGroup.end());
            }
        }
    }

    // Edge groups for dependent choices
    //
    for(vector<int> const& group : Constraints::get_dependent_choices(
            _inputData->assignment_constraints(),
            _inputData->choice_count()))
    {
        if(group.size() == 1) continue;

        for(int p = 0; p < _inputData->chooser_count(); p++)
        {
            vector<flowid> edgeGroup;
            for(int w : group)
            {
                int s = scheduling->slot_of(w);
                int from = flow.nodes().at(MipFlowStaticData::node_chooser(p, s));
                int to = flow.nodes().at(MipFlowStaticData::node_choice(w));
                edgeGroup.push_back(MipFlowStaticData::edge_id(from, to));
            }

            flow.make_edges_equal(edgeGroup.begin(), edgeGroup.end());
        }
    }
}

const_ptr<Assignment> AssignmentSolver::solve_with_limit(const_ptr<Scheduling> const& scheduling,
                                                                int preferenceLimit,
                                                                op::MPSolver& solver)
{
    MipFlow<flowid, flowid> flow(_staticData->baseFlow);

    for(int p = 0; p < _inputData->chooser_count(); p++)
    {
        for(int s = 0; s < _inputData->slot_count(); s++)
        {
            flow.set_supply(flow.nodes().at(MipFlowStaticData::node_chooser(p, s)), 1);
        }
    }

    for(int w = 0; w < _inputData->choice_count(); w++)
    {
        flow.set_supply(flow.nodes().at(MipFlowStaticData::node_choice(w)), -_inputData->choice(w).min);
    }

    for(int s = 0; s < _inputData->slot_count(); s++)
    {
        // Count the number of choosers that will already be absorbed by the choice nodes.
        //
        int coveredChoosers = 0;
        for(int w = 0; w < _inputData->choice_count(); w++)
        {
            if(scheduling->slot_of(w) != s) continue;
            coveredChoosers += _inputData->choice(w).min;
        }

        flow.set_supply(
                flow.nodes().at(MipFlowStaticData::node_slot(s)),
                -(_inputData->chooser_count() - coveredChoosers));
    }

    vector<int> edgesCap;
    vector<long> edgesCost;
    map<pair<int, int>, int> edgesIdx;
    int nextEdgeIdx = 0;

    for(int p = 0; p < _inputData->chooser_count(); p++)
    {
        for(int s = 0; s < _inputData->slot_count(); s++)
        {
            for(int w = 0; w < _inputData->choice_count(); w++)
            {
                if(scheduling->slot_of(w) != s || _inputData->chooser(p).preferences[w] > preferenceLimit)
                    continue;

                edgesCap.push_back(1);
                edgesCost.push_back(
                        (long)pow(_inputData->chooser(p).preferences[w] + 1.0, _options->preference_exponent()));

                edgesIdx[std::make_pair(
                        flow.nodes().at(MipFlowStaticData::node_chooser(p, s)),
                        flow.nodes().at(MipFlowStaticData::node_choice(w)))]
                        = nextEdgeIdx++;
            }
        }
    }

    for(int w = 0; w < _inputData->choice_count(); w++)
    {
        for(int s = 0; s < _inputData->slot_count(); s++)
        {
            if(scheduling->slot_of(w) != s) continue;

            edgesCap.push_back(_inputData->choice(w).max - _inputData->choice(w).min);
            edgesCost.push_back(0);

            edgesIdx[std::make_pair(
                    flow.nodes().at(MipFlowStaticData::node_choice(w)),
                    flow.nodes().at(MipFlowStaticData::node_slot(s)))]
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
    create_implications(scheduling, flow);

    // ... and solve this instance
    //
    if(!flow.solve(solver))
    {
        return nullptr;
    }

    // Now we have to extract the assignment solution from the min cost flow solution
    //
    vector<vector<int>> data(_inputData->chooser_count(), vector<int>(_inputData->slot_count(), -1));
    for(int p = 0; p < _inputData->chooser_count(); p++)
    {
        for(int s = 0; s < _inputData->slot_count(); s++)
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

    _lpCount++;
    return std::make_shared<Assignment const>(_inputData, data);
}

const_ptr<Assignment> AssignmentSolver::solve(const_ptr<Scheduling const> const& scheduling)
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

    if(!_options->greedy())
    {
        // We do binary search through all possible preference limits to find the lowest one.
        //
        do
        {
            int prefLimit = _inputData->preference_levels().at(prefIdx);
            auto assignment = solve_with_limit(scheduling, prefLimit, solver);
            if (assignment == nullptr)
            {
                minIdx = prefIdx + 1;
            }
            else
            {
                bestAssignment = assignment;
                maxIdx = prefIdx - 1;
            }
            prefIdx = (maxIdx + minIdx) / 2;
        } while (maxIdx > minIdx);
    }
    else
    {
        // In greedy mode, we don't set a preference limit; just solve it.
        //
        bestAssignment = solve_with_limit(scheduling, _inputData->max_preference(), solver);
    }

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

int AssignmentSolver::lp_count() const
{
    return _lpCount;
}

void AssignmentSolver::handle_chooser_is_in_choice(Constraint constraint, const_ptr<Scheduling> const& scheduling,
                                                   MipFlow<flowid, flowid>& flow)
{
    int s = scheduling->slot_of(constraint.right());
    int from = _staticData->baseFlow.nodes().at(MipFlowStaticData::node_chooser(constraint.left(), s));

    for(int w = 0; w < _inputData->choice_count(); w++)
    {
        if(constraint.right() == w || scheduling->slot_of(w) != s) continue;

        int to = _staticData->baseFlow.nodes().at(MipFlowStaticData::node_choice(w));
        flow.block_edge(MipFlowStaticData::edge_id(from, to));
    }
}

void AssignmentSolver::handle_chooser_is_not_in_choice(Constraint constraint, const_ptr<Scheduling> const& scheduling,
                                                       MipFlow<flowid, flowid>& flow)
{
    for(int s = 0; s < _inputData->slot_count(); s++)
    {
        int from = _staticData->baseFlow.nodes().at(MipFlowStaticData::node_chooser(constraint.left(), s));
        int to = _staticData->baseFlow.nodes().at(MipFlowStaticData::node_choice(constraint.right()));
        flow.block_edge(MipFlowStaticData::edge_id(from, to));
    }
}

void AssignmentSolver::handle_choices_of_choosers_relation(Constraint constraint,
                                                           const_ptr<Scheduling> const& scheduling,
                                                           MipFlow<flowid, flowid>& flow)
{
    for(int s = 0; s < _inputData->slot_count(); s++)
    {
        int fromLeft = MipFlowStaticData::node_chooser(constraint.left(), s);
        int fromRight = MipFlowStaticData::node_chooser(constraint.right(), s);

        for(int w = 0; w < _inputData->choice_count(); w++)
        {
            int to = MipFlowStaticData::node_choice(w);
            int fromEdge = flow.edges().at(MipFlowStaticData::edge_id(fromLeft, to));
            int toEdge = flow.edges().at(MipFlowStaticData::edge_id(fromRight, to));

            switch(constraint.extra())
            {
                case Subset:
                    flow.add_implication(fromEdge, toEdge);
                    break;
                case Superset:
                    flow.add_implication(toEdge, fromEdge);
                    break;
                case Eq:
                    flow.add_implication(fromEdge, toEdge);
                    flow.add_implication(toEdge, fromEdge);
                    break;
            }
        }
    }
}

void AssignmentSolver::handle_choosers_of_choices_relation(Constraint constraint,
                                                           const_ptr<Scheduling> const& scheduling,
                                                      MipFlow<flowid, flowid>& flow)
{
    for(int )
}

void AssignmentSolver::handle_constraints(const_ptr<Scheduling> const& scheduling, MipFlow<flowid, flowid>& flow)
{
}
