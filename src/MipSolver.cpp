#include "MipSolver.h"

#include "Status.h"
#include "SchedulingSolver.h"
#include "Constraints.h"
#include "Score.h"

#include <climits>
#include <utility>

#ifndef USE_CBC
#define USE_CBC
#endif

#define SLOT_ID_HIGH (INT_MAX / 2)
#define WORKSHOP_ID_HIGH (INT_MAX / 2 - 1)

#include <ortools/linear_solver/linear_solver.h>


flowid MipSolver::make_long(int high, int low) { return ((flowid)high << 32U) | ((flowid)low & 0xFFFFFFFFUL); }

flowid MipSolver::node_participant(int p, int s) { return make_long(p, s); }

flowid MipSolver::node_slot(int s) { return make_long(SLOT_ID_HIGH, s); }

flowid MipSolver::node_workshop(int w) { return make_long(WORKSHOP_ID_HIGH, w); }

flowid MipSolver::edge_id(int from, int to) { return make_long(from, to); }

void MipSolver::check_for_possible_overflow(InputData const& inputData)
{
    if(std::pow((double)inputData.max_preference(), _options.preference_exponent()) * inputData.participant_count() >= LONG_MAX)
    {
        Status::warning("The preference exponent is too large; computations may cause an integer overflow");
    }

}

CriticalSetAnalysis MipSolver::get_cs_analysis(InputData const& inputData)
{
    if(!_options.no_critical_sets() && inputData.slot_count() > 1)
    {
        Status::info("Performing critical set analysis.");
        CriticalSetAnalysis cs(inputData);
        Status::info(str(cs.sets().size()) + " critical set(s) found; preference limit is " + str(cs.preference_bound()) + ".");
        return cs;
    }
    else
    {
        if(inputData.slot_count() > 1)
        {
            Status::info("Skipping critical set analysis.");
        }
        else
        {
            Status::info("No critical set analysis needed for single slot.");
        }
        return CriticalSetAnalysis::empty(inputData);
    }
}

MipFlowStaticData MipSolver::get_static_graph_data(InputData const& inputData)
{
    MipFlowStaticData data{};

    for(int p = 0; p < inputData.participant_count(); p++)
    {
        for(int s = 0; s < inputData.slot_count(); s++)
        {
            data.baseFlow.add_node(node_participant(p, s));
        }
    }

    for(int w = 0; w < inputData.workshop_count(); w++)
    {
        data.baseFlow.add_node(node_workshop(w));
    }

    for(int s = 0; s < inputData.slot_count(); s++)
    {
        data.baseFlow.add_node(node_slot(s));
    }

    data.constraints = inputData.assignment_constraints();

    return data;
}

op::MPSolver MipSolver::new_solver(int tid)
{
    return op::MPSolver("solver" + str(tid), op::MPSolver::CBC_MIXED_INTEGER_PROGRAMMING);
}

set<pair<int, int>> MipSolver::get_blocked_constraint_edges(InputData const& inputData, Scheduling const& scheduling,
                                                            MipFlowStaticData const& staticData)
{
    set<pair<int, int>> blockedEdges;

    for(Constraint constraint : staticData.constraints)
    {
        switch(constraint.type())
        {
            case ParticipantIsInWorkshop:
            {
                int s = scheduling.slot_of(constraint.right());
                int from = staticData.baseFlow.nodes().at(node_participant(constraint.left(), s));

                for(int w = 0; w < inputData.workshop_count(); w++)
                {
                    if(constraint.right() == w || scheduling.slot_of(w) != s) continue;

                    int to = staticData.baseFlow.nodes().at(node_workshop(w));
                    blockedEdges.insert(std::make_pair(from, to));
                }
                break;
            }
            case ParticipantIsNotInWorkshop:
            {
                for(int s = 0; s < inputData.slot_count(); s++)
                {
                    int from = staticData.baseFlow.nodes().at(node_participant(constraint.left(), s));
                    int to = staticData.baseFlow.nodes().at(node_workshop(constraint.right()));
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

Solution MipSolver::solve_assignment(InputData const& inputData, op::MPSolver& solver,
                                     shared_ptr<Scheduling const> const& scheduling,
                                     CriticalSetAnalysis const& csAnalysis, MipFlowStaticData const& staticData)
{
    int prefIdx = 0;
    for(; prefIdx < inputData.preference_levels().size(); prefIdx++)
    {
        if(inputData.preference_levels().at(prefIdx) == csAnalysis.preference_bound()) break;
    }

    int minIdx = prefIdx;
    int maxIdx = inputData.preference_levels().size();

    Solution bestSol;
    Solution sol;

    do
    {
        int prefLimit = inputData.preference_levels().at(prefIdx);
        sol = solve_assignment(inputData, solver, scheduling, staticData, prefLimit);
        if(sol.is_invalid())
        {
            minIdx = prefIdx + 1;
        }
        else
        {
            bestSol = sol;
            maxIdx = prefIdx - 1;
        }
        prefIdx = (maxIdx + minIdx) / 2;
    } while(maxIdx > minIdx);

    if(bestSol.is_invalid())
    {
        return Solution::invalid();
    }

    return bestSol;
}

Solution MipSolver::solve_assignment(InputData const& inputData, op::MPSolver& solver,
                                     shared_ptr<Scheduling const> const& scheduling,
                                     MipFlowStaticData const& staticData, int preferenceLimit)
{
    MipFlow<flowid, flowid> flow(staticData.baseFlow);

    for(int p = 0; p < inputData.participant_count(); p++)
    {
        for(int s = 0; s < inputData.slot_count(); s++)
        {
            flow.add_supply(flow.nodes().at(node_participant(p, s)), 1);
        }
    }

    for(int w = 0; w < inputData.workshop_count(); w++)
    {
        flow.add_supply(flow.nodes().at(node_workshop(w)), -inputData.workshop(w).min());
    }

    for(int s = 0; s < inputData.slot_count(); s++)
    {
        // Count the number of participants that will already be absorbed by the workshop nodes.
        //
        int coveredParticipants = 0;
        for(int w = 0; w < inputData.workshop_count(); w++)
        {
            if(scheduling->slot_of(w) != s) continue;
            coveredParticipants += inputData.workshop(w).min();
        }

         flow.add_supply(flow.nodes().at(node_slot(s)), -(inputData.participant_count() - coveredParticipants));
    }

    vector<int> edgesCap;
    vector<long> edgesCost;
    map<pair<int, int>, int> edgesIdx;
    int nextEdgeIdx = 0;

    for(int p = 0; p < inputData.participant_count(); p++)
    {
        for(int s = 0; s < inputData.slot_count(); s++)
        {
            for(int w = 0; w < inputData.workshop_count(); w++)
            {
                if(scheduling->slot_of(w) != s || inputData.participant(p).preference(w) > preferenceLimit)
                    continue;

                edgesCap.push_back(1);
                edgesCost.push_back((long)pow(inputData.participant(p).preference(w) + 1.0, _options.preference_exponent()));

                edgesIdx[std::make_pair(flow.nodes().at(node_participant(p, s)), flow.nodes().at(node_workshop(w)))] = nextEdgeIdx++;
            }
        }
    }

    for(int w = 0; w < inputData.workshop_count(); w++)
    {
        for(int s = 0; s < inputData.slot_count(); s++)
        {
            if(scheduling->slot_of(w) != s) continue;

            edgesCap.push_back(inputData.workshop(w).max() - inputData.workshop(w).min());
            edgesCost.push_back(0);

            edgesIdx[std::make_pair(flow.nodes().at(node_workshop(w)), flow.nodes().at(node_slot(s)))] = nextEdgeIdx++;
        }
    }

    // Remove all blocked edges
    //
    auto blockedEdges = get_blocked_constraint_edges(inputData, *scheduling, staticData);
    blockedEdges.insert(staticData.blockedEdges.begin(), staticData.blockedEdges.end());

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

        flow.add_edge(edge_id(start, end), start, end, cap, cost);
    }

    // Create edge groups
    //
    for(vector<int> const& group : Constraints::get_dependent_workshops(inputData.assignment_constraints(), inputData.workshop_count()))
    {
        if(group.size() == 1) continue;

        for(int p = 0; p < inputData.participant_count(); p++)
        {
            vector<long> edgeGroup;
            for(int w : group)
            {
                int s = scheduling->slot_of(w);
                int from = flow.nodes().at(node_participant(p, s));
                int to = flow.nodes().at(node_workshop(w));
                edgeGroup.push_back(edge_id(from, to));
            }

            flow.create_edge_group_or_block_edges(edgeGroup.begin(), edgeGroup.end());
        }
    }

    // ... and solve this instance
    //
    datetime startTime = time_now();
    bool solverStatus = flow.solve(solver);
    datetime endTime = time_now();

    _solveTimeMutex.lock();
    _solveTime.push_back(std::chrono::duration_cast<secondsf>(endTime - startTime).count());
    _solveTimeMutex.unlock();

    if(!solverStatus)
    {
        return Solution::invalid();
    }

    // Now we have to extract the assignment solution from the min cost flow solution
    //
    vector<vector<int>> data(inputData.participant_count(), vector<int>(inputData.slot_count(), -1));
    for(int p = 0; p < inputData.participant_count(); p++)
    {
        for(int s = 0; s < inputData.slot_count(); s++)
        {
            for(int w = 0; w < inputData.workshop_count(); w++)
            {
                int from = flow.nodes().at(node_participant(p, s));
                int to = flow.nodes().at(node_workshop(w));

                if(flow.solution_value_at(edge_id(from, to)) == 1)
                {
                    data[p][s] = w;
                }
            }
        }
    }

    return Solution(scheduling, std::make_shared<Assignment const>(inputData, data));
}

shared_ptr<Scheduling const>
MipSolver::feasible_neighbor(InputData const& inputData, shared_ptr<Scheduling const> const& scheduling)
{
    const int MAX_TRIES = 1000;
    vector<int> data(scheduling->raw_data());

    int tries = 0;
    while(true)
    {
        if(tries++ > MAX_TRIES)
        {
            return nullptr;
        }

        int w = Rng::next(0, inputData.workshop_count());
        int s = Rng::next(0, inputData.slot_count());

        if(scheduling->slot_of(w) == s) continue;

        int origs = data[w];
        data[w] = s;
        auto newScheduling = std::make_shared<Scheduling const>(inputData, data);
        data[w] = origs;

        if(!newScheduling->is_feasible()) continue;

        return newScheduling;
    }
}

void MipSolver::do_shotgun_hill_climbing(int tid, InputData const& inputData, CriticalSetAnalysis const& csAnalysis,
                                         MipFlowStaticData const& staticData, Scoring const& scoring,
                                         map<Scheduling, Solution>& doneSchedulings,
                                         std::shared_mutex& doneSchedulingsMutex,
                                         std::shared_future<void> exitSignal)
{
    auto solver = new_solver(tid);

    SchedulingSolver schedulingSolver(inputData, csAnalysis, _options, exitSignal);

    while (schedulingSolver.next_scheduling())
    {
        if(exitSignal.valid() && exitSignal.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) return;

        _tries++;

        auto schedulingPtr = schedulingSolver.scheduling();
        Solution localBestSolution = Solution::invalid();

        Score localBestScore{.major = INFINITY, .minor = INFINITY};

        while(true)
        {
            shared_ptr<Scheduling const> nextSchedulingPtr;
            if(localBestSolution.is_invalid())
            {
                nextSchedulingPtr = schedulingPtr;
            }
            else
            {
                nextSchedulingPtr = feasible_neighbor(inputData, schedulingPtr);
            }

            if(nextSchedulingPtr == nullptr)
            {
                break;
            }

            if(exitSignal.valid() && exitSignal.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) return;

            doneSchedulingsMutex.lock_shared();
            auto foundSolutionIt = doneSchedulings.find(*nextSchedulingPtr);
            doneSchedulingsMutex.unlock_shared();

            Solution foundSolution = Solution::invalid();
            if(foundSolutionIt == doneSchedulings.end())
            {
                auto newSolution = solve_assignment(inputData, solver, nextSchedulingPtr, csAnalysis, staticData);
                if(newSolution.is_invalid())
                {
                    break;
                }

                foundSolution = newSolution;
                doneSchedulingsMutex.lock();
                doneSchedulings[*nextSchedulingPtr] = foundSolution;
                doneSchedulingsMutex.unlock();
            }
            else
            {
                foundSolution = foundSolutionIt->second;
            }

            Score foundScore = scoring.evaluate(foundSolution);

            if(foundScore < localBestScore)
            {
                localBestSolution = foundSolution;
                localBestScore = foundScore;
                schedulingPtr = nextSchedulingPtr;

                if(foundScore < _bestsScore[tid])
                {
                    _bests[tid] = localBestSolution;
                    _bestsScore[tid] = foundScore;
                }

                continue;
            }

            break;
        }
    }
}

Solution MipSolver::best_solution_found()
{
    int bestIdx = -1;
    for(int i = 0; i < _bests.size(); i++)
    {
        if(bestIdx == -1 || _bestsScore[i] < _bestsScore[bestIdx])
        {
            bestIdx = i;
        }
    }

    return _bests[bestIdx];
}

MipSolver::MipSolver(Options const& options)
    : _options(options)
{
}

Solution MipSolver::solve(InputData const& inputData)
{
    Status::error("a");
    check_for_possible_overflow(inputData);

    Status::error("b");
    CriticalSetAnalysis csAnalysis = get_cs_analysis(inputData);
    MipFlowStaticData staticData = get_static_graph_data(inputData);

    Status::error("c");
    if(_options.any() || inputData.slot_count() == 1)
    {
        Status::info("Computing solution.");
        SchedulingSolver schedulingSolver(inputData, csAnalysis, _options);

        if(!schedulingSolver.next_scheduling())
        {
            return Solution::invalid();
        }

        auto solver = new_solver();
        return solve_assignment(inputData, solver, schedulingSolver.scheduling(), csAnalysis, staticData);
    }
    else
    {
        Status::info("Started min cost flow solver with " + str(_options.thread_count()) + " thread(s).");
        Scoring scoring(inputData, _options);

        Status::error("d");
        _bests.clear();
        _bestsScore.clear();
        _threads.clear();

        Status::error("e");
        _bests.resize(_options.thread_count());
        _bestsScore.resize(_options.thread_count());
        _threads.resize(_options.thread_count());

        Status::error("f");
        std::fill(_bestsScore.begin(), _bestsScore.end(), Score{.major = INFINITY, .minor = INFINITY});

        Status::error("g");
        map<Scheduling, Solution> doneSchedulings;
        std::shared_mutex doneSchedulingsMutex;

        Status::error("g");
        std::promise<void> exitSignal;
        std::shared_future<void> exitFuture = exitSignal.get_future().share();

        Status::error("h");
        for(int tid = 0; tid < _threads.size(); tid++)
        {
            Status::error("thread " + str(tid));
            _threads[tid] = thread([&, tid]()
                                   {
                                       do_shotgun_hill_climbing(
                                               tid,
                                               inputData,
                                               csAnalysis,
                                               staticData,
                                               scoring,
                                               doneSchedulings,
                                               doneSchedulingsMutex,
                                               exitFuture);
                                   });
        }

        Status::error("i");
        datetime startTime = time_now();
        seconds timeout(_options.timeout_seconds());

        Score lastScore{.major = INFINITY, .minor = INFINITY};

        while(time_now() < startTime + timeout)
        {
            sleep(1);

            Score bestScore = *std::min_element(_bestsScore.begin(), _bestsScore.end());
            string newString = bestScore != lastScore ? "NEW" : "   ";
            lastScore = bestScore;

            _solveTimeMutex.lock();
            if(_solveTime.empty())
            {
                _solveTime.push_back(0);
            }

            double minTime = *std::min_element(_solveTime.begin(), _solveTime.end());
            double maxTime = *std::max_element(_solveTime.begin(), _solveTime.end());

            doneSchedulingsMutex.lock_shared();
            Status::info(newString
                         + " BEST=" + bestScore.to_str()
                         + ", ETA=" + str((startTime + timeout) - time_now())
                         + ", TRIES=" + str(_tries)
                         + "(" + str(doneSchedulings.size())
                         + "), STIME=" + str(minTime, 3)
                         + "s-" + str(maxTime, 3)
                         + "s");

            _solveTime.clear();
            doneSchedulingsMutex.unlock_shared();
            _solveTimeMutex.unlock();
        }

        exitSignal.set_value();
        Status::info("Stopped min cost flow solver. Waiting for workers to finish.");

        for(thread& thread : _threads)
        {
            thread.join();
        }

        return best_solution_found();
    }
}
