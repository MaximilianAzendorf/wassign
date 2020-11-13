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
            case ParticipantIsInWorkshop:
            {
                int s = scheduling->slot_of(constraint.right());
                int from = _staticData->baseFlow.nodes().at(MipFlowStaticData::node_participant(constraint.left(), s));

                for(int w = 0; w < _inputData->workshop_count(); w++)
                {
                    if(constraint.right() == w || scheduling->slot_of(w) != s) continue;

                    int to = _staticData->baseFlow.nodes().at(MipFlowStaticData::node_workshop(w));
                    blockedEdges.insert(std::make_pair(from, to));
                }
                break;
            }
            case ParticipantIsNotInWorkshop:
            {
                for(int s = 0; s < _inputData->slot_count(); s++)
                {
                    int from = _staticData->baseFlow.nodes().at(MipFlowStaticData::node_participant(constraint.left(), s));
                    int to = _staticData->baseFlow.nodes().at(MipFlowStaticData::node_workshop(constraint.right()));
                    blockedEdges.insert(std::make_pair(from, to));
                }
                break;
            }
            case WorkshopsHaveSameParticipants:
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

    for(int p = 0; p < _inputData->participant_count(); p++)
    {
        for(int s = 0; s < _inputData->slot_count(); s++)
        {
            flow.add_supply(flow.nodes().at(MipFlowStaticData::node_participant(p, s)), 1);
        }
    }

    for(int w = 0; w < _inputData->workshop_count(); w++)
    {
        flow.add_supply(flow.nodes().at(MipFlowStaticData::node_workshop(w)), -_inputData->workshop(w).min());
    }

    for(int s = 0; s < _inputData->slot_count(); s++)
    {
        // Count the number of participants that will already be absorbed by the workshop nodes.
        //
        int coveredParticipants = 0;
        for(int w = 0; w < _inputData->workshop_count(); w++)
        {
            if(scheduling->slot_of(w) != s) continue;
            coveredParticipants += _inputData->workshop(w).min();
        }

        flow.add_supply(
                flow.nodes().at(MipFlowStaticData::node_slot(s)),
                -(_inputData->participant_count() - coveredParticipants));
    }

    vector<int> edgesCap;
    vector<long> edgesCost;
    map<pair<int, int>, int> edgesIdx;
    int nextEdgeIdx = 0;

    for(int p = 0; p < _inputData->participant_count(); p++)
    {
        for(int s = 0; s < _inputData->slot_count(); s++)
        {
            for(int w = 0; w < _inputData->workshop_count(); w++)
            {
                if(scheduling->slot_of(w) != s || _inputData->participant(p).preference(w) > preferenceLimit)
                    continue;

                edgesCap.push_back(1);
                edgesCost.push_back(
                        (long)pow(_inputData->participant(p).preference(w) + 1.0, _options->preference_exponent()));

                edgesIdx[std::make_pair(
                        flow.nodes().at(MipFlowStaticData::node_participant(p, s)),
                        flow.nodes().at(MipFlowStaticData::node_workshop(w)))]
                        = nextEdgeIdx++;
            }
        }
    }

    for(int w = 0; w < _inputData->workshop_count(); w++)
    {
        for(int s = 0; s < _inputData->slot_count(); s++)
        {
            if(scheduling->slot_of(w) != s) continue;

            edgesCap.push_back(_inputData->workshop(w).max() - _inputData->workshop(w).min());
            edgesCost.push_back(0);

            edgesIdx[std::make_pair(
                    flow.nodes().at(MipFlowStaticData::node_workshop(w)),
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
    for(vector<int> const& group : Constraints::get_dependent_workshops(
            _inputData->assignment_constraints(),
            _inputData->workshop_count()))
    {
        if(group.size() == 1) continue;

        for(int p = 0; p < _inputData->participant_count(); p++)
        {
            vector<long> edgeGroup;
            for(int w : group)
            {
                int s = scheduling->slot_of(w);
                int from = flow.nodes().at(MipFlowStaticData::node_participant(p, s));
                int to = flow.nodes().at(MipFlowStaticData::node_workshop(w));
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
    vector<vector<int>> data(_inputData->participant_count(), vector<int>(_inputData->slot_count(), -1));
    for(int p = 0; p < _inputData->participant_count(); p++)
    {
        for(int s = 0; s < _inputData->slot_count(); s++)
        {
            for(int w = 0; w < _inputData->workshop_count(); w++)
            {
                int from = flow.nodes().at(MipFlowStaticData::node_participant(p, s));
                int to = flow.nodes().at(MipFlowStaticData::node_workshop(w));

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
